/*
 * ScoringService.cpp
 */

#include "modules/scoring/ScoringService.hpp"

#include <algorithm>
#include <string>
#include <utility>

#include "utils/Time.hpp"

namespace softadastra::cloud::modules::scoring
{
  ScoringService::ScoringService(
      const softadastra::cloud::app::AppState &state)
      : state_(state),
        run_repository_(state.database),
        event_repository_(state.database)
  {
  }

  ScoringResult ScoringService::compute_for_run(
      const std::string &run_public_id)
  {
    if (run_public_id.empty())
    {
      return validation_error("Run id is required");
    }

    const auto run =
        run_repository_.find_by_public_id(run_public_id);

    if (!run.has_value())
    {
      return not_found("Run not found");
    }

    const std::int64_t data_loss_events =
        event_repository_.count_by_type_for_run(
            run_public_id,
            "data_loss_risk_detected");

    const std::int64_t duplicate_events =
        event_repository_.count_by_type_for_run(
            run_public_id,
            "duplicate_detected");

    const std::int64_t timeout_events =
        event_repository_.count_by_type_for_run(
            run_public_id,
            "request_timeout");

    const std::int64_t recovery_failed_events =
        event_repository_.count_by_type_for_run(
            run_public_id,
            "recovery_failed");

    const std::int64_t critical_events =
        event_repository_.count_by_severity_for_run(
            run_public_id,
            "critical");

    const std::int64_t error_events =
        event_repository_.count_by_severity_for_run(
            run_public_id,
            "error");

    const std::int64_t warning_events =
        event_repository_.count_by_severity_for_run(
            run_public_id,
            "warning");

    int score = 100;

    if (data_loss_events > 0)
    {
      score -= 35;
    }

    if (duplicate_events > 0)
    {
      score -= 20;
    }

    if (timeout_events > 0)
    {
      score -= 10;
    }

    if (recovery_failed_events > 0)
    {
      score -= 25;
    }

    score -= static_cast<int>(
        std::min<std::int64_t>(critical_events * 15, 30));

    score -= static_cast<int>(
        std::min<std::int64_t>(error_events * 8, 24));

    score -= static_cast<int>(
        std::min<std::int64_t>(warning_events * 3, 15));

    ReliabilityScore reliability_score;

    reliability_score.run_id = run->id;
    reliability_score.run_public_id = run->public_id;
    reliability_score.score = clamp_score(score);
    reliability_score.grade =
        grade_for_score(reliability_score.score);

    reliability_score.data_loss_risk = data_loss_events > 0;
    reliability_score.duplicate_risk = duplicate_events > 0;
    reliability_score.timeout_risk = timeout_events > 0;
    reliability_score.recovery_risk = recovery_failed_events > 0;
    reliability_score.created_at =
        softadastra::cloud::utils::Time::now_ms();

    ScoringResult result;
    result.success = true;
    result.status = 200;
    result.error.clear();
    result.message = "Reliability score computed";
    result.score = reliability_score;

    return result;
  }

  int ScoringService::clamp_score(int score) noexcept
  {
    return std::clamp(score, 0, 100);
  }

  std::string ScoringService::grade_for_score(int score)
  {
    if (score >= 90)
    {
      return "A";
    }

    if (score >= 75)
    {
      return "B";
    }

    if (score >= 60)
    {
      return "C";
    }

    if (score >= 40)
    {
      return "D";
    }

    return "F";
  }

  ScoringResult ScoringService::validation_error(
      std::string message)
  {
    ScoringResult result;
    result.success = false;
    result.status = 400;
    result.error = "validation_failed";
    result.message = std::move(message);
    return result;
  }

  ScoringResult ScoringService::not_found(
      std::string message)
  {
    ScoringResult result;
    result.success = false;
    result.status = 404;
    result.error = "run_not_found";
    result.message = std::move(message);
    return result;
  }

} // namespace softadastra::cloud::modules::scoring
