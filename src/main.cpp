/**
 * @file main.cpp
 * @brief Entry point for the cloud backend application.
 */

#include <cloud/app/AppBootstrap.hpp>

/**
 * @brief Start the backend application.
 *
 * The main function stays intentionally small. Application setup,
 * middleware registration, route registration, and server startup are
 * delegated to cloud::app::AppBootstrap.
 *
 * @return Process exit code.
 */
int main()
{
  cloud::app::AppBootstrap bootstrap;
  return bootstrap.run();
}
