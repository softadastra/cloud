#include <feedback/services/FeedbackService.hpp>
#include <notifications/services/NotificationService.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <vix/config/Config.hpp>
#include <vix/db/db.hpp>

namespace cloud::feedback::services {
namespace {
std::int64_t now_ts(){ return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }
std::string make_id(){ static std::atomic<std::uint64_t> c{0}; std::ostringstream s; s<<"feedback_"<<now_ts()<<"_"<<c.fetch_add(1); return s.str(); }
bool valid_category(const std::string &v){ return v=="bug"||v=="idea"||v=="improvement"||v=="question"||v=="other"; }
bool valid_status(const std::string &v){ return v=="open"||v=="reviewed"||v=="planned"||v=="closed"; }
}

class FeedbackService::Impl {
public:
  Impl(){ const char *u=std::getenv("SOFTADASTRA_CLOUD_USE_DB"); if(u&&std::string(u)=="1"){ vix::config::Config cfg{".env"}; db=std::make_unique<vix::db::Database>(cfg);} }
  bool persistent() const { return db!=nullptr; }
  dto::FeedbackResponse row(const vix::db::ResultRow &r) const { dto::FeedbackResponse f; f.id=r.getString(0); f.user_id=r.getString(1); f.workspace_id=r.getString(2); f.category=r.getString(3); f.title=r.getString(4); f.message=r.getString(5); f.status=r.getString(6); f.created_at=r.getInt64(7); f.updated_at=r.getInt64(8); f.deleted_at=r.getInt64(9); return f; }
  std::unique_ptr<vix::db::Database> db;
};

FeedbackService::FeedbackService(): impl_(std::make_unique<Impl>()){}
FeedbackService::~FeedbackService()=default;

FeedbackResult<dto::FeedbackResponse> FeedbackService::create(const dto::CreateFeedbackRequest &q){
  if(q.user_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_user","User is required.");
  if(q.title.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_title","Feedback title is required.");
  if(q.message.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_message","Feedback message is required.");
  const auto category=q.category.empty()?"other":q.category;
  if(!valid_category(category)) return FeedbackResult<dto::FeedbackResponse>::failure("invalid_category","Feedback category is invalid.");
  dto::FeedbackResponse f; f.id=make_id(); f.user_id=q.user_id; f.workspace_id=q.workspace_id; f.category=category; f.title=q.title; f.message=q.message; f.status="open"; f.created_at=now_ts(); f.updated_at=f.created_at;
  if(!impl_->persistent()) return FeedbackResult<dto::FeedbackResponse>::success(f);
  impl_->db->exec("INSERT INTO feedback_items (id,user_id,workspace_id,category,title,message,status,created_at,updated_at) VALUES (?,?,?,?,?,?,?,?,?)", f.id,f.user_id,f.workspace_id,f.category,f.title,f.message,f.status,f.created_at,f.updated_at);
  cloud::notifications::services::NotificationService notifications; cloud::notifications::dto::CreateNotificationRequest n; n.recipient_user_id=f.user_id; n.workspace_id=f.workspace_id; n.actor_user_id=f.user_id; n.type="feedback_received"; n.title="Your feedback was received"; n.message="Thanks. Your feedback was sent."; n.data_json="{}"; notifications.create(n);
  return FeedbackResult<dto::FeedbackResponse>::success(f);
}

FeedbackResult<std::vector<dto::FeedbackResponse>> FeedbackService::list_mine(const dto::ListFeedbackRequest &q) const {
  if(q.user_id.empty()) return FeedbackResult<std::vector<dto::FeedbackResponse>>::failure("missing_user","User is required.");
  std::vector<dto::FeedbackResponse> out; if(!impl_->persistent()) return FeedbackResult<std::vector<dto::FeedbackResponse>>::success(out);
  auto rows=impl_->db->query("SELECT id,user_id,COALESCE(workspace_id,''),category,title,message,status,created_at,updated_at,COALESCE(deleted_at,0) FROM feedback_items WHERE user_id = ? AND COALESCE(deleted_at,0)=0 ORDER BY created_at DESC LIMIT ?", q.user_id, static_cast<std::int64_t>(q.limit>0&&q.limit<=200?q.limit:50));
  while(rows->next()) out.push_back(impl_->row(rows->row()));
  return FeedbackResult<std::vector<dto::FeedbackResponse>>::success(out);
}

FeedbackResult<std::vector<dto::FeedbackResponse>> FeedbackService::list_workspace(const dto::ListFeedbackRequest &q) const {
  if(q.workspace_id.empty()) return FeedbackResult<std::vector<dto::FeedbackResponse>>::failure("missing_workspace","Workspace is required.");
  std::vector<dto::FeedbackResponse> out; if(!impl_->persistent()) return FeedbackResult<std::vector<dto::FeedbackResponse>>::success(out);
  auto rows=impl_->db->query("SELECT id,user_id,COALESCE(workspace_id,''),category,title,message,status,created_at,updated_at,COALESCE(deleted_at,0) FROM feedback_items WHERE workspace_id = ? AND COALESCE(deleted_at,0)=0 ORDER BY created_at DESC LIMIT ?", q.workspace_id, static_cast<std::int64_t>(q.limit>0&&q.limit<=200?q.limit:50));
  while(rows->next()) out.push_back(impl_->row(rows->row()));
  return FeedbackResult<std::vector<dto::FeedbackResponse>>::success(out);
}

FeedbackResult<dto::FeedbackResponse> FeedbackService::update_status(const dto::UpdateFeedbackStatusRequest &q){
  if(q.feedback_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_feedback","Feedback is required.");
  if(!valid_status(q.status)) return FeedbackResult<dto::FeedbackResponse>::failure("invalid_status","Feedback status is invalid.");
  impl_->db->exec("UPDATE feedback_items SET status = ?, updated_at = ? WHERE id = ? AND COALESCE(deleted_at,0)=0", q.status, now_ts(), q.feedback_id);
  auto rows=impl_->db->query("SELECT id,user_id,COALESCE(workspace_id,''),category,title,message,status,created_at,updated_at,COALESCE(deleted_at,0) FROM feedback_items WHERE id = ? AND COALESCE(deleted_at,0)=0 LIMIT 1", q.feedback_id);
  if(!rows->next()) return FeedbackResult<dto::FeedbackResponse>::failure("feedback_not_found","Feedback not found.");
  auto f=impl_->row(rows->row());
  cloud::notifications::services::NotificationService notifications; cloud::notifications::dto::CreateNotificationRequest n; n.recipient_user_id=f.user_id; n.workspace_id=f.workspace_id; n.actor_user_id=q.actor_user_id; n.type="feedback_status_changed"; n.title="Your feedback was updated"; n.message="Your feedback was marked as " + f.status + "."; n.data_json="{}"; notifications.create(n);
  return FeedbackResult<dto::FeedbackResponse>::success(f);
}

FeedbackResult<dto::FeedbackResponse> FeedbackService::update_own(const dto::UpdateFeedbackRequest &q){
  if(q.feedback_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_feedback","Feedback is required.");
  if(q.user_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_user","User is required.");
  if(q.title.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_title","Feedback title is required.");
  if(q.message.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_message","Feedback message is required.");
  const auto category=q.category.empty()?"other":q.category;
  if(!valid_category(category)) return FeedbackResult<dto::FeedbackResponse>::failure("invalid_category","Feedback category is invalid.");
  auto current=impl_->db->query("SELECT status FROM feedback_items WHERE id = ? AND user_id = ? AND COALESCE(deleted_at,0)=0 LIMIT 1", q.feedback_id,q.user_id);
  if(!current->next()) return FeedbackResult<dto::FeedbackResponse>::failure("feedback_not_found","Feedback not found.");
  if(current->row().getString(0)!="open") return FeedbackResult<dto::FeedbackResponse>::failure("feedback_not_open","Only open feedback can be edited.");
  impl_->db->exec("UPDATE feedback_items SET category=?, title=?, message=?, updated_at=? WHERE id=? AND user_id=?", category,q.title,q.message,now_ts(),q.feedback_id,q.user_id);
  auto rows=impl_->db->query("SELECT id,user_id,COALESCE(workspace_id,''),category,title,message,status,created_at,updated_at,COALESCE(deleted_at,0) FROM feedback_items WHERE id = ? AND user_id = ? LIMIT 1", q.feedback_id,q.user_id);
  if(!rows->next()) return FeedbackResult<dto::FeedbackResponse>::failure("feedback_not_found","Feedback not found.");
  return FeedbackResult<dto::FeedbackResponse>::success(impl_->row(rows->row()));
}

FeedbackResult<dto::FeedbackResponse> FeedbackService::close_own(const std::string &feedback_id,const std::string &user_id){
  if(feedback_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_feedback","Feedback is required.");
  if(user_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_user","User is required.");
  impl_->db->exec("UPDATE feedback_items SET status='closed', updated_at=? WHERE id=? AND user_id=? AND COALESCE(deleted_at,0)=0", now_ts(),feedback_id,user_id);
  auto rows=impl_->db->query("SELECT id,user_id,COALESCE(workspace_id,''),category,title,message,status,created_at,updated_at,COALESCE(deleted_at,0) FROM feedback_items WHERE id = ? AND user_id = ? AND COALESCE(deleted_at,0)=0 LIMIT 1", feedback_id,user_id);
  if(!rows->next()) return FeedbackResult<dto::FeedbackResponse>::failure("feedback_not_found","Feedback not found.");
  return FeedbackResult<dto::FeedbackResponse>::success(impl_->row(rows->row()));
}

FeedbackResult<dto::FeedbackResponse> FeedbackService::delete_own(const std::string &feedback_id,const std::string &user_id){
  if(feedback_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_feedback","Feedback is required.");
  if(user_id.empty()) return FeedbackResult<dto::FeedbackResponse>::failure("missing_user","User is required.");
  const auto ts=now_ts();
  impl_->db->exec("UPDATE feedback_items SET deleted_at=?, updated_at=? WHERE id=? AND user_id=? AND COALESCE(deleted_at,0)=0", ts,ts,feedback_id,user_id);
  dto::FeedbackResponse f; f.id=feedback_id; f.user_id=user_id; f.status="deleted"; f.deleted_at=ts;
  return FeedbackResult<dto::FeedbackResponse>::success(f);
}
}
