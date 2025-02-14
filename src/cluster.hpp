#pragma once
#include "shared.hpp"

class DeathLocationStack {
public:
	std::vector<DeathLocation*> deaths;
	CCPoint center;
	float diameter = 0;
	float density = 0;

	DeathLocationStack(std::vector<DeathLocation*> deaths);

	void recalculate();
};

void identifyClusters(std::vector<DeathLocation>* const deaths, float maxDistance, std::vector<DeathLocationStack>* stacks);