#pragma once
#include "shared.hpp"

class DeathLocationStack {
public:
	std::vector<DeathLocation> deaths;
	CCPoint center;
	float diameter = 0;

	DeathLocationStack(std::vector<DeathLocation> deaths);

	void recalculate();
};

CCPoint averagePos(std::vector<DeathLocation>::iterator const begin, std::vector<DeathLocation>::iterator const end);

void identifyClusters(std::vector<DeathLocation>* deaths, float maxDistance, std::vector<DeathLocationStack>* stacks);