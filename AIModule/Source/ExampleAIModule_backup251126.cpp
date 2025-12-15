#include "ExampleAIModule.h"
#include "ontology.h"	// add_251013
#include <iostream>
#include <Windows.h>

using namespace BWAPI;
using namespace Filter;

// 전역 컨테이너 생성 add_251013
std::vector<Ontology::Player> playersSnapshot;
std::vector<Ontology::Unit> unitsSnapshot;
// 전역 컨테이너 생성 add_251013


void ExampleAIModule::onStart()
{
	// 콘솔 창 생성 add_251013
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	// 콘솔 창 생성 add_251013

	std::cout << "=== Ontology Console Log Started ===" << std::endl;

	Broodwar->sendText("Hello world!");

	Broodwar << "The map is " << Broodwar->mapName() << "!" << std::endl;

	Broodwar->enableFlag(Flag::UserInput);

	Broodwar->setCommandOptimizationLevel(2);

	if (Broodwar->isReplay())
	{
		Broodwar << "The following players are in this replay:" << std::endl;

		Playerset players = Broodwar->getPlayers();
		for (auto p : players)
		{
			if (!p->isObserver())
				Broodwar << p->getName() << ", playing as " << p->getRace() << std::endl;
		}

	}
	else
	{
		if (Broodwar->enemy()) 
			Broodwar << "The matchup is " << Broodwar->self()->getRace() << " vs " << Broodwar->enemy()->getRace() << std::endl;
	}

	Ontology::Player selfPlayer;
	selfPlayer.id = "P1";
	selfPlayer.name = Broodwar->self()->getName();
	selfPlayer.faction = Ontology::Faction::Self;
	selfPlayer.supplyUsed = Broodwar->self()->supplyUsed();
	selfPlayer.supplyCap = Broodwar->self()->supplyTotal();

	Broodwar->printf("Player %s, Supply: %d/%d",
		selfPlayer.name.c_str(),
		selfPlayer.supplyUsed,
		selfPlayer.supplyCap
	);
}

void ExampleAIModule::onEnd(bool isWinner)
{
	if (isWinner)
	{
		// dummy
	}
}

void ExampleAIModule::onFrame()
{
	Broodwar->drawTextScreen(200, 0, "FPS: %d", Broodwar->getFPS());
	Broodwar->drawTextScreen(200, 20, "Average FPS: %f", Broodwar->getAverageFPS());

	if (Broodwar->isReplay() || Broodwar->isPaused() || !Broodwar->self())
		return;

	if (Broodwar->getFrameCount() % Broodwar->getLatencyFrames() != 0)
		return;

	for (auto &u : Broodwar->self()->getUnits())
	{
		if (!u->exists())
			continue;

		if (u->isLockedDown() || u->isMaelstrommed() || u->isStasised())
			continue;

		if (u->isLoaded() || !u->isPowered() || u->isStuck())
			continue;

		if (!u->isCompleted() || u->isConstructing())
			continue;

		if (u->getType().isWorker())
		{
			if (u->isIdle())
			{
				if (u->isCarryingGas() || u->isCarryingMinerals())
				{
					u->returnCargo();
				}
				else if (!u->getPowerUp())  
				{                            
					if (!u->gather(u->getClosestUnit(IsMineralField || IsRefinery)))
					{
						Broodwar << Broodwar->getLastError() << std::endl;
					}

				} 
			} 

		}
		else if (u->getType().isResourceDepot()) 
		{

			if (u->isIdle() && !u->train(u->getType().getRace().getWorker()))
			{
				Position pos = u->getPosition();
				Error lastErr = Broodwar->getLastError();
				Broodwar->registerEvent([pos, lastErr](Game*) { Broodwar->drawTextMap(pos, "%c%s", Text::White, lastErr.c_str()); }, 
					nullptr,    
					Broodwar->getLatencyFrames());  

				UnitType supplyProviderType = u->getType().getRace().getSupplyProvider();
				static int lastChecked = 0;

				if (lastErr == Errors::Insufficient_Supply &&
					lastChecked + 400 < Broodwar->getFrameCount() &&
					Broodwar->self()->incompleteUnitCount(supplyProviderType) == 0)
				{
					lastChecked = Broodwar->getFrameCount();

					Unit supplyBuilder = u->getClosestUnit(GetType == supplyProviderType.whatBuilds().first &&
						(IsIdle || IsGatheringMinerals) &&
						IsOwned);
					if (supplyBuilder)
					{
						if (supplyProviderType.isBuilding())
						{
							TilePosition targetBuildLocation = Broodwar->getBuildLocation(supplyProviderType, supplyBuilder->getTilePosition());
							if (targetBuildLocation)
							{
								Broodwar->registerEvent([targetBuildLocation, supplyProviderType](Game*)
								{
									Broodwar->drawBoxMap(Position(targetBuildLocation),
										Position(targetBuildLocation + supplyProviderType.tileSize()),
										Colors::Blue);
								},
									nullptr, 
									supplyProviderType.buildTime() + 100); 

								supplyBuilder->build(supplyProviderType, targetBuildLocation);
							}
						}
						else
						{
							supplyBuilder->train(supplyProviderType);
						}
					} 
				} 
			} 

		}

	}


	// add_251013
	// === Ontology 데이터 수집 블록 ===
	int currentFrame = Broodwar->getFrameCount();

	playersSnapshot.clear();
	for (auto p : Broodwar->getPlayers()) {
		if (!p->isObserver()) {
			Ontology::Player player;
			player.id = "P" + std::to_string(p->getID());
			player.name = p->getName();

			if (p == Broodwar->self()) player.faction = Ontology::Faction::Self;
			else if (p->isEnemy(Broodwar->self())) player.faction = Ontology::Faction::Enemy;
			else if (p->isAlly(Broodwar->self())) player.faction = Ontology::Faction::Ally;
			else player.faction = Ontology::Faction::Neutral;

			player.supplyUsed = p->supplyUsed();
			player.supplyCap = p->supplyTotal();
			player.techLevel = 0; // (필요시 확장)

			playersSnapshot.push_back(player);
		}
	}

	unitsSnapshot.clear();
	for (auto u : Broodwar->getAllUnits()) {
		Ontology::Unit myUnit;
		myUnit.id = "P" + std::to_string(u->getPlayer()->getID()) + ":U" + std::to_string(u->getID());
		myUnit.type = u->getType().getName();
		myUnit.ownerId = "P" + std::to_string(u->getPlayer()->getID());

		myUnit.hpCurrent = u->getHitPoints();
		myUnit.hpMax = u->getType().maxHitPoints();
		myUnit.shieldCurrent = u->getShields();
		myUnit.shieldMax = u->getType().maxShields();
		myUnit.energy = u->getEnergy();

		myUnit.posTile = { u->getTilePosition().x, u->getTilePosition().y };
		myUnit.posPixel = { u->getPosition().x, u->getPosition().y };

		if (u->isAttacking())      myUnit.status = Ontology::UnitStatus::Attacking;
		else if (u->isMoving())    myUnit.status = Ontology::UnitStatus::Moving;
		else if (u->isIdle())      myUnit.status = Ontology::UnitStatus::Idle;
		else                       myUnit.status = Ontology::UnitStatus::Holding;

		myUnit.sightRange = u->getType().sightRange();
		myUnit.weapon.name = u->getType().groundWeapon().getName();
		myUnit.weapon.range = u->getType().groundWeapon().maxRange();
		myUnit.weapon.cooldown = u->getGroundWeaponCooldown();

		myUnit.lastSeenFrame = currentFrame;
		myUnit.visibility = (u->exists() && u->isVisible()) ? Ontology::Visibility::Visible : Ontology::Visibility::Hidden;

		if (u->getTarget()) {
			myUnit.targetId = "P" + std::to_string(u->getTarget()->getPlayer()->getID()) + ":U" + std::to_string(u->getTarget()->getID());
		}
		else {
			myUnit.targetId = "";
		}

		unitsSnapshot.push_back(myUnit);
	}

	// Debug: 콘솔 출력
	for (const auto &pl : playersSnapshot) {
		std::cout << "[Frame " << currentFrame << "] Player " << pl.name
			<< " Supply " << pl.supplyUsed << "/" << pl.supplyCap << std::endl;
	}
	for (const auto &unit : unitsSnapshot) {
		std::cout << "[Frame " << currentFrame << "] " << unit.type
			<< " HP:" << unit.hpCurrent << "/" << unit.hpMax
			<< " Pos:(" << unit.posTile.x << "," << unit.posTile.y << ")"
			<< std::endl;
	}
	// add_251013
}

void ExampleAIModule::onSendText(std::string text)
{
	Broodwar->sendText("%s", text.c_str());
}

void ExampleAIModule::onReceiveText(BWAPI::Player player, std::string text)
{
	Broodwar << player->getName() << " said \"" << text << "\"" << std::endl;
}

void ExampleAIModule::onPlayerLeft(BWAPI::Player player)
{
	Broodwar->sendText("Goodbye %s!", player->getName().c_str());
}

void ExampleAIModule::onNukeDetect(BWAPI::Position target)
{
	if (target)
	{
		Broodwar << "Nuclear Launch Detected at " << target << std::endl;
	}
	else
	{
		Broodwar->sendText("Where's the nuke?");
	}
}

void ExampleAIModule::onUnitDiscover(BWAPI::Unit unit)
{
}

void ExampleAIModule::onUnitEvade(BWAPI::Unit unit)
{
}

void ExampleAIModule::onUnitShow(BWAPI::Unit unit)
{
}

void ExampleAIModule::onUnitHide(BWAPI::Unit unit)
{
}

void ExampleAIModule::onUnitCreate(BWAPI::Unit unit)
{
	if (Broodwar->isReplay())
	{
		if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
		{
			int seconds = Broodwar->getFrameCount() / 24;
			int minutes = seconds / 60;
			seconds %= 60;
			Broodwar->sendText("%.2d:%.2d: %s creates a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
		}
	}
}

void ExampleAIModule::onUnitDestroy(BWAPI::Unit unit)
{
}

void ExampleAIModule::onUnitMorph(BWAPI::Unit unit)
{
	if (Broodwar->isReplay())
	{
		if (unit->getType().isBuilding() && !unit->getPlayer()->isNeutral())
		{
			int seconds = Broodwar->getFrameCount() / 24;
			int minutes = seconds / 60;
			seconds %= 60;
			Broodwar->sendText("%.2d:%.2d: %s morphs a %s", minutes, seconds, unit->getPlayer()->getName().c_str(), unit->getType().c_str());
		}
	}
}

void ExampleAIModule::onUnitRenegade(BWAPI::Unit unit)
{
}

void ExampleAIModule::onSaveGame(std::string gameName)
{
	Broodwar << "The game was saved to \"" << gameName << "\"" << std::endl;
}

void ExampleAIModule::onUnitComplete(BWAPI::Unit unit)
{
}