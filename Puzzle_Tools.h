#pragma once
#include "Puzzle.h"
struct ImagePuzzle;
class Camera;

void arrange(Puzzle &puzzle, bool edge, bool animate = true);
void reset_view(Puzzle &puzzle, Camera &camera);

void drag_tool(Puzzle &puzzle, Camera &camera, Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet, const PuzzleCoords &rel, const ScreenCoords &dst, P2d &autoscroll, double mdx = 0, double mdy = 0);
bool drag_tool_drop(Puzzle &puzzle, Camera &camera, Puzzle::Piece piece, std::set<Puzzle::Piece> &magnet);
inline void drag_tool(Puzzle &puzzle, Camera &camera, Puzzle::Piece piece, const PuzzleCoords &rel, const ScreenCoords &dst, P2d &v, double mdx = 0, double mdy = 0) { std::set<Puzzle::Piece> dummy; ::drag_tool(puzzle, camera, piece, dummy, rel, dst, v, mdx, mdy); }
inline bool drag_tool_drop(Puzzle &puzzle, Camera &camera, Puzzle::Piece piece) { std::set<Puzzle::Piece> dummy; return ::drag_tool_drop(puzzle, camera, piece, dummy); }

void hide_tool(ImagePuzzle &puzzle, Puzzle::Piece piece, bool and_similar = false);
void shovel_tool(Puzzle &puzzle, Camera &camera, const ScreenCoords &dst, const ScreenCoords &delta);
