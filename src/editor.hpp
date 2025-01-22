#pragma once
#include "shared.hpp"

class DeathsAnalyser {
protected:
	DeathsAnalyser(std::vector<DeathLocation>& deaths) {
		this->deaths = deaths;
	}

	std::vector<DeathLocation> deaths;

public:
	void setDeaths(std::vector<DeathLocation>& deaths) {
		this->deaths = deaths;
	}
};