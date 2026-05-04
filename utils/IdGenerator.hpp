/**
 *
 *  @file IdGenerator.hpp
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

#ifndef SOFTADASTRA_CLOUD_UTILS_ID_GENERATOR_HPP
#define SOFTADASTRA_CLOUD_UTILS_ID_GENERATOR_HPP

#include <string>
#include <string_view>

namespace softadastra::cloud::utils
{
  /**
   * @brief Public id generator for Softadastra Cloud.
   *
   * IdGenerator creates stable public ids used by API responses and routes.
   *
   * Internal database ids stay private. Public ids use readable prefixes:
   * - usr_xxx
   * - prj_xxx
   * - agt_xxx
   * - run_xxx
   * - evt_xxx
   * - rpt_xxx
   */
  class IdGenerator final
  {
  public:
    /**
     * @brief Generates a public id with a prefix.
     *
     * @param prefix Public id prefix.
     * @return Generated id.
     */
    [[nodiscard]] static std::string generate(
        std::string_view prefix);

    /**
     * @brief Generates a compact random suffix.
     *
     * @return Random suffix.
     */
    [[nodiscard]] static std::string random_suffix();

  private:
    IdGenerator() = delete;
  };

} // namespace softadastra::cloud::utils

#endif // SOFTADASTRA_CLOUD_UTILS_ID_GENERATOR_HPP
