#pragma once
#include "shared.hpp"
#include "smallestCircle.hpp"

class DeathLocationStack {
public:
	std::vector<DeathLocation*> deaths;
	Circle circle;
	float density = 0;
	
	DeathLocationStack();
	DeathLocationStack(std::vector<DeathLocation*> deaths);

	void recalculate();
};

void identifyClusters(std::vector<DeathLocation>* const deaths,
	float maxDistance, std::vector<DeathLocationStack>* stacks);