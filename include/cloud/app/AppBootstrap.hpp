/**
 * @file AppBootstrap.hpp
 * @brief Application bootstrap for the cloud backend.
 */

#ifndef VIX_GENERATED_BACKEND_CLOUD_APP_BOOTSTRAP_HPP
#define VIX_GENERATED_BACKEND_CLOUD_APP_BOOTSTRAP_HPP

namespace cloud::app
{
  /**
   * @brief Owns the startup sequence of the backend application.
   *
   * AppBootstrap keeps main.cpp minimal and centralizes the application
   * initialization flow: configuration loading, Vix app creation,
   * middleware registration, route registration, and server startup.
   */
  class AppBootstrap
  {
  public:
    /**
     * @brief Create a default application bootstrap instance.
     */
    AppBootstrap() = default;

    /**
     * @brief Destroy the application bootstrap instance.
     */
    ~AppBootstrap() = default;

    AppBootstrap(const AppBootstrap &) = delete;
    AppBootstrap &operator=(const AppBootstrap &) = delete;
    AppBootstrap(AppBootstrap &&) = delete;
    AppBootstrap &operator=(AppBootstrap &&) = delete;

    /**
     * @brief Run the backend application.
     *
     * @return Process exit code. Returns 0 when the application exits normally.
     */
    int run();
  };
} // namespace cloud::app

#endif // VIX_GENERATED_BACKEND_CLOUD_APP_BOOTSTRAP_HPP
