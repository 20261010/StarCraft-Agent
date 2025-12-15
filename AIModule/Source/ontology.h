#pragma once
#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>

namespace Ontology{
	enum class Faction { Self, Ally, Enemy, Neutral };
	enum class UnitStatus { Idle, Moving, Attacking, Holding, Gathering, Building, Repairing, Casting, Fleeing };
	enum class Visibility { Visible, Hidden, Unknown };
	enum class ResourceType { Mineral, Gas };
	enum class MapFeatureType { Region, ChokePoint, BaseLocation };
	enum class SquadRole { Scout, Defense, Assault, Harass };

	struct PositionTile {
		int x;
		int y;
	};

	struct PositionPixel {
		int x;
		int y;
	};

	struct Player {
		std::string id;
		std::string name;
		Faction faction;
		int supplyUsed;
		int supplyCap;
		int techLevel;
	};

	struct Unit {
		std::string id;          
		std::string type;       
		std::string ownerId;

		int hpCurrent;
		int hpMax;
		int shieldCurrent;
		int shieldMax;
		int energy;

		PositionTile posTile;
		PositionPixel posPixel;

		UnitStatus status;
		int sightRange;

		struct {
			std::string name;
			int range;
			int cooldown;
		} weapon;

		int lastSeenFrame;
		Visibility visibility;
		std::string targetId;
	};

	struct Building {
		std::string id;
		std::string type;
		std::string ownerId;

		PositionTile posTile;
		int hpCurrent;
		int hpMax;

		std::vector<std::string> productionQueue;
	};

	struct Resource {
		std::string id;
		ResourceType type;
		int amount;
		PositionTile posTile;
	};

	struct MapFeature {
		std::string id;
		MapFeatureType type;
		std::vector<PositionTile> polygon;
		PositionTile center;
	};

	struct Contact {
		std::string id;
		std::string kind;       
		PositionPixel position;
		double confidence;      
		int firstSeenFrame;
		int lastSeenFrame;
	};

	struct Squad {
		std::string id;
		SquadRole role;
		std::vector<std::string> memberUnitIds;
		PositionPixel centroid;
		double strengthScore;
	};

	struct EngagementRule {
		std::string id;
		std::string name;
		int priority;
		std::vector<std::string> appliesTo; 

		std::string condition; 
		std::string action;    
	};
}