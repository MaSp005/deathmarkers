#include "cluster.hpp"

// i could write a fucking paper about this file

DeathLocationStack::DeathLocationStack(std::vector<DeathLocation> deaths) {
	this->deaths = deaths;
	this->recalculate();
}

void DeathLocationStack::recalculate() {
	this->diameter = 0;
	this->center = averagePos(this->deaths.begin(), this->deaths.end());

	for (auto& death : this->deaths) {
		auto dist = this->center.getDistanceSq(death.pos);
		if (dist > this->diameter)
			this->diameter = dist;
	}

	this->diameter = sqrt(this->diameter) * 2;
}


void mergeStacks(std::vector<DeathLocationStack>* stacks, int a, int b) {
	// Append b's deaths onto a
	stacks->at(a).deaths.reserve(stacks->at(a).deaths.size() + stacks->at(b).deaths.size());
	stacks->at(a).deaths.insert(stacks->at(a).deaths.end(), stacks->at(b).deaths.begin(), stacks->at(b).deaths.end());
	// Recalculate a
	stacks->at(a).recalculate();
	// Remove b from vector
	stacks->erase(stacks->begin() + b);
}

CCPoint averagePos(std::vector<DeathLocation>::iterator const begin, std::vector<DeathLocation>::iterator const end) {
	double avgX = 0;
	double avgY = 0;

	for (auto death = begin; death < end; death++) {
		avgX += death->pos.x;
		avgY += death->pos.y;
	}

	avgX /= end - begin;
	avgY /= end - begin;

	return CCPoint(avgX, avgY);
}

int findNearest(std::vector<DeathLocationStack> const& stacks, int sourceIndex, float maxDistance) {
	// Assumes death stacks vector is (roughly) sorted by x-coordinate

	CCPoint srcPoint = stacks.at(sourceIndex).center;
	float minDist = maxDistance;
	int minimum = -1;
	
	// Walk right
	for (auto i = stacks.begin() + sourceIndex + 1; i < stacks.end(); i++) {
		// All points from here will be out of range on x alone, skip
		if (i->center.x > srcPoint.x + maxDistance) break;
		float dist = i->center.getDistanceSq(srcPoint);
		if (dist < minDist) {
			minimum = i - stacks.begin();
			minDist = dist;
		}
	}
	// Walk left
	for (auto i = stacks.begin() + sourceIndex - 1; i >= stacks.begin(); i--) {
		if (i->center.x < srcPoint.x - maxDistance) break;
		float dist = i->center.getDistanceSq(srcPoint);
		if (dist < minDist) {
			minimum = i - stacks.begin();
			minDist = dist;
		}
		if (i == stacks.begin()) break;
	}

	minDist = sqrt(minDist);
	// log::info("Found nearest to {}: {}, distance {}", sourceIndex, minimum, minDist);

	if (minDist > maxDistance) return -1;
	return minimum;
}

void identifyClusters(std::vector<DeathLocation>* const deaths, float maxDistance, std::vector<DeathLocationStack>* stacks) {

	/*
	*  Hierarchical Clustering
	*  -----------------------
	*  Turn each node into a single-element cluster, repeatedly merge all clusters until no more merging can take place
	*  Assumes deaths vector is sorted by x-coordinate
	*/

	log::info("Clustering {} entries with maximum distance {}", deaths->size(), maxDistance);
	stacks->clear();
	stacks->reserve(deaths->size());

	for (auto i = deaths->begin(); i < deaths->end(); i++) {
		std::vector<DeathLocation> vector;
		vector.push_back(*i);
		stacks->push_back(DeathLocationStack(vector));
	}
	// At this point, death stacks vector is also sorted by x-coordinate
	
	int iter = 0;
	while (true) {
		bool merged = false;
		iter++;
		// log::info("Iteration {}, {} stacks", iter, stacks->size());

		for (auto i = stacks->begin(); i < stacks->end(); i++) {
			int nearest = findNearest(*stacks, i - stacks->begin(), maxDistance);
			if (nearest == -1) continue;
			merged = true;
			mergeStacks(stacks, i - stacks->begin(), nearest);
		}

		if (!merged) break;
	}

	std::erase_if(*stacks, [](const auto& stack) { return stack.deaths.size() == 1; });

	log::info("Finished clustering into {} stacks.", stacks->size());
}