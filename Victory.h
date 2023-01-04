#pragma once
struct Puzzle;
class  Camera;

class VictoryAnimation
{
public:
	VictoryAnimation(Puzzle &puzzle, Camera &camera);
	void run(double dt);
	bool done() const;

protected:
	Puzzle &puzzle;
	Camera &camera;
	double t;
};
