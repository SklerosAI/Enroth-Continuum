#pragma once

#include <memory>

class GraphicsImage;

void GameOver_Setup();
std::unique_ptr<GraphicsImage> CreateWinnerCertificate();

/**
 * Draws MM6's win screen with the party, its play time and its score, and saves it as `MM6_Win.Pcx`.
 *
 * @return                              The drawn screen.
 */
std::unique_ptr<GraphicsImage> CreateMM6WinScreen();

/**
 * Draws MM6's losing screen.
 *
 * @return                              The drawn screen.
 */
std::unique_ptr<GraphicsImage> CreateMM6LoseScreen();
