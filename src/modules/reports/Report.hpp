/**
 *
 *  @file Report.hpp
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

#ifndef SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_HPP
#define SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_HPP

#include <cstdint>
#include <string>

namespace softadastra::cloud::modules::reports
{
  /**
   * @brief Report model for a reliability test run.
   *
   * A report is the final structured result produced by an agent after a run.
   * It stores both a human-readable summary and the raw JSON report payload.
   */
  struct Report
  {
    std::int64_t id{0};
    std::string public_id{};

    std::int64_t run_id{0};
    std::string run_public_id{};

    std::string summary{};
    std::string raw_json{};

    std::int64_t created_at{0};
    std::int64_t updated_at{0};

    [[nodiscard]] bool is_valid() const noexcept
    {
      return id > 0 &&
             !public_id.empty() &&
             run_id > 0 &&
             !summary.empty() &&
             !raw_json.empty();
    }

    [[nodiscard]] bool has_run() const noexcept
    {
      return run_id > 0 || !run_public_id.empty();
    }

    [[nodiscard]] bool has_raw_json() const noexcept
    {
      return !raw_json.empty();
    }
  };

} // namespace softadastra::cloud::modules::reports

#endif // SOFTADASTRA_CLOUD_MODULES_REPORTS_REPORT_HPP
