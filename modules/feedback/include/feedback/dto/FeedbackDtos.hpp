#ifndef cloud_feedback_dtos_hpp
#define cloud_feedback_dtos_hpp
#include <cstdint>
#include <string>
#include <vix/json.hpp>
namespace cloud::feedback::dto {
struct FeedbackResponse {
  std::string id,user_id,workspace_id,category,title,message,status;
  std::int64_t created_at{0},updated_at{0},deleted_at{0};
  vix::json::Json to_json() const { return vix::json::o("id",id,"user_id",user_id,"workspace_id",workspace_id,"category",category,"title",title,"message",message,"status",status,"created_at",created_at,"updated_at",updated_at,"deleted_at",deleted_at); }
};
struct CreateFeedbackRequest { std::string user_id,workspace_id,category,title,message; };
struct ListFeedbackRequest { std::string user_id,workspace_id; int limit{50}; };
struct UpdateFeedbackStatusRequest { std::string feedback_id,workspace_id,actor_user_id,status; };
struct UpdateFeedbackRequest { std::string feedback_id,user_id,category,title,message; };
}
#endif
