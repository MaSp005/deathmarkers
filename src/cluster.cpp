#include "cluster.hpp"

DeathLocationStack::DeathLocationStack(std::vector<DeathLocation*> deaths) {
	this->deaths = deaths;
	this->recalculate();
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
void DeathLocationStack::recalculate() {
	this->diameter = 0;
	this->center = averagePos(this->deaths.begin(), this->deaths.end());

	for (auto death : this->deaths) {
		auto dist = this->center.getDistanceSq(death->pos);
		if (dist > this->diameter)
			this->diameter = dist;
	}

	this->diameter = sqrt(this->diameter) * 2;

	this->density = static_cast<float>(this->deaths.size()) / (this->diameter ? this->diameter * this->diameter : 1);
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
void mergeStacks(std::vector<DeathLocationStack>* stacks, std::vector<DeathLocationStack>::iterator a, std::vector<DeathLocationStack>::iterator b) {
	// Append b's deaths onto a
	a->deaths.reserve(a->deaths.size() + b->deaths.size());
	a->deaths.insert(a->deaths.end(), b->deaths.begin(), b->deaths.end());
	// Recalculate a
	a->recalculate();
	// Remove b from vector
	stacks->erase(b);
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
CCPoint averagePos(std::vector<DeathLocation*>::iterator const begin, std::vector<DeathLocation*>::iterator const end) {
	double avgX = 0;
	double avgY = 0;

	for (auto death = begin; death < end; death++) {
		avgX += (*death)->pos.x;
		avgY += (*death)->pos.y;
	}

	avgX /= end - begin;
	avgY /= end - begin;

	return CCPoint(avgX, avgY);
}

// Time complexity O(n)
// Auxiliary space complexity O(1)
int findNearest(std::vector<DeathLocationStack> const* stacks, std::vector<DeathLocationStack>::iterator source, float maxDistance) {
	// Assumes death stacks vector is (roughly) sorted by x-coordinate

	CCPoint srcPoint = source->center;
	float minDistSq = maxDistance * maxDistance;
	int minimum = -1;
	
	// Walk right
	for (auto i = source + 1; i < stacks->end(); i++) {
		// All points from here will be out of range on x alone, skip
		if (i->center.x > srcPoint.x + maxDistance) break;
		float dist = i->center.getDistanceSq(srcPoint);
		if (dist < minDistSq) {
			minimum = i - stacks->begin();
			minDistSq = dist;
		}
	}
	// Walk left
	for (auto i = source - 1; i >= stacks->begin(); i--) {
		if (i->center.x < srcPoint.x - maxDistance) break;
		float dist = i->center.getDistanceSq(srcPoint);
		if (dist < minDistSq) {
			minimum = i - stacks->begin();
			minDistSq = dist;
		}
		if (i == stacks->begin()) break;
	}

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
		std::vector<DeathLocation*> vector;
		vector.push_back(&*i);
		stacks->push_back(DeathLocationStack(vector));
	}
	// At this point, death stacks vector is also sorted by x-coordinate
	
	int deathIdx = -1;
	int iter = 0;
	while (true) {
		bool merged = false;
		iter++;

		for (auto i = stacks->begin(); i < stacks->end();) {
			deathIdx++;
			int nearest = findNearest(stacks, i, maxDistance);
			if (nearest == -1) {
				if (i->deaths.size() <= 1) {
					// This can only happen in the first iteration, otherwise it will have been merged and have > 1 elements
					(*i->deaths.begin())->clustered = false;
					stacks->erase(i);
					continue;
				}
				i++;
				continue;
			}
			merged = true;
			mergeStacks(stacks, stacks->begin(), stacks->begin() + nearest);
		}

		if (!merged) break;
	}

	std::erase_if(*stacks, [maxDistance](const DeathLocationStack& stack) {
		if (stack.deaths.size() == 1) return true;
		// if (stack.diameter == 0) return false;
		// TODO: some kind of density check to disallow clusters of non-related deaths
		return false;
	});

	log::info("Finished clustering into {} stacks.", stacks->size());
}