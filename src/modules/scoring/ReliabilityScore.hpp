/**
 *
 *  @file ReliabilityScore.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2026, Softadastra.
 *  All rights reserved.
 *  https://github.com/softadastra/cloud
 *
 *  Licensed under the Apache License, Version 2.0.
 *
 *  Softadastra Cloud
 *
 */

#ifndef SOFTADASTRA_CLOUD_MODULES_SCORING_RELIABILITY_SCORE_HPP
#define SOFTADASTRA_CLOUD_MODULES_SCORING_RELIABILITY_SCORE_HPP

#include <cstdint>
#include <string>

namespace softadastra::cloud::modules::scoring
{
  /**
   * @brief Reliability score model for a run.
   *
   * ReliabilityScore represents the computed risk profile of one reliability
   * test run.
   *
   * The score is intentionally simple for the MVP:
   * - 100 means excellent reliability
   * - lower values indicate higher reliability risk
   */
  struct ReliabilityScore
  {
    std::int64_t id{0};

    std::int64_t run_id{0};
    std::string run_public_id{};

    int score{100};
    std::string grade{"A"};

    bool data_loss_risk{false};
    bool duplicate_risk{false};
    bool timeout_risk{false};
    bool recovery_risk{false};

    std::int64_t created_at{0};

    [[nodiscard]] bool is_valid() const noexcept
    {
      return run_id > 0 &&
             score >= 0 &&
             score <= 100 &&
             !grade.empty();
    }

    [[nodiscard]] bool has_run() const noexcept
    {
      return run_id > 0 || !run_public_id.empty();
    }

    [[nodiscard]] bool has_any_risk() const noexcept
    {
      return data_loss_risk ||
             duplicate_risk ||
             timeout_risk ||
             recovery_risk;
    }

    [[nodiscard]] bool is_excellent() const noexcept
    {
      return score >= 90;
    }

    [[nodiscard]] bool is_good() const noexcept
    {
      return score >= 75 && score < 90;
    }

    [[nodiscard]] bool is_risky() const noexcept
    {
      return score >= 50 && score < 75;
    }

    [[nodiscard]] bool is_dangerous() const noexcept
    {
      return score < 50;
    }
  };

} // namespace softadastra::cloud::modules::scoring

#endif // SOFTADASTRA_CLOUD_MODULES_SCORING_RELIABILITY_SCORE_HPP
