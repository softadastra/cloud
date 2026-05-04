/**
 *
 *  @file ScoringService.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_SCORING_SCORING_SERVICE_HPP
#define SOFTADASTRA_CLOUD_MODULES_SCORING_SCORING_SERVICE_HPP

#include <cstdint>
#include <string>

#include "app/AppState.hpp"
#include "modules/events/EventRepository.hpp"
#include "modules/runs/RunRepository.hpp"
#include "modules/scoring/ReliabilityScore.hpp"

namespace softadastra::cloud::modules::scoring
{
  /**
   * @brief Result returned by scoring operations.
   */
  struct ScoringResult
  {
    bool success{false};
    int status{400};

    std::string error{"scoring_error"};
    std::string message{};

    ReliabilityScore score{};
  };

  /**
   * @brief Reliability scoring service.
   *
   * ScoringService computes a simple reliability score from the events
   * attached to one run.
   *
   * It does not read Vix Request and does not write Vix Response.
   *
   * MVP scoring rules:
   * - start from 100
   * - subtract points when risky event types are present
   * - clamp final score between 0 and 100
   * - map score to a grade
   */
  class ScoringService final
  {
  public:
    /**
     * @brief Creates a scoring service from shared app state.
     *
     * @param state Shared application state.
     */
    explicit ScoringService(
        const softadastra::cloud::app::AppState &state);

    /**
     * @brief Computes a reliability score for one run.
     *
     * @param run_public_id Public run id.
     * @return Scoring result.
     */
    [[nodiscard]] ScoringResult compute_for_run(
        const std::string &run_public_id);

  private:
    const softadastra::cloud::app::AppState &state_;
    softadastra::cloud::modules::runs::RunRepository run_repository_;
    softadastra::cloud::modules::events::EventRepository event_repository_;

    [[nodiscard]] static int clamp_score(int score) noexcept;

    [[nodiscard]] static std::string grade_for_score(
        int score);

    [[nodiscard]] static ScoringResult validation_error(
        std::string message);

    [[nodiscard]] static ScoringResult not_found(
        std::string message);
  };

} // namespace softadastra::cloud::modules::scoring

#endif // SOFTADASTRA_CLOUD_MODULES_SCORING_SCORING_SERVICE_HPP
