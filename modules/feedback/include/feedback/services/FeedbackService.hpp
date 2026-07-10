#ifndef cloud_feedback_service_hpp
#define cloud_feedback_service_hpp
#include <feedback/dto/FeedbackDtos.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace cloud::feedback::services {
template <typename T> class FeedbackResult { public: static FeedbackResult success(T v){ return FeedbackResult(true,std::move(v),{},{}); } static FeedbackResult failure(std::string c,std::string m){ return FeedbackResult(false,T{},std::move(c),std::move(m)); } bool ok() const{return ok_;} bool failed() const{return !ok_;} const T &value() const{return value_;} T &value(){return value_;} const std::string &error_code() const{return code_;} const std::string &error_message() const{return message_;} private: FeedbackResult(bool ok,T v,std::string c,std::string m):ok_(ok),value_(std::move(v)),code_(std::move(c)),message_(std::move(m)){} bool ok_; T value_{}; std::string code_,message_; };
class FeedbackService { public: FeedbackService(); ~FeedbackService(); FeedbackResult<dto::FeedbackResponse> create(const dto::CreateFeedbackRequest &request); FeedbackResult<std::vector<dto::FeedbackResponse>> list_mine(const dto::ListFeedbackRequest &request) const; FeedbackResult<std::vector<dto::FeedbackResponse>> list_workspace(const dto::ListFeedbackRequest &request) const; FeedbackResult<dto::FeedbackResponse> update_status(const dto::UpdateFeedbackStatusRequest &request); FeedbackResult<dto::FeedbackResponse> update_own(const dto::UpdateFeedbackRequest &request); FeedbackResult<dto::FeedbackResponse> close_own(const std::string &feedback_id,const std::string &user_id); FeedbackResult<dto::FeedbackResponse> delete_own(const std::string &feedback_id,const std::string &user_id); private: class Impl; std::unique_ptr<Impl> impl_; };
}
#endif
