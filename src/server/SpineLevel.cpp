/*
	This file is part of Spine.

    Spine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Spine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Spine.  If not, see <http://www.gnu.org/licenses/>.
 */
// Copyright 2020 Clockwork Origins

#include "SpineLevel.h"

#include "MariaDBWrapper.h"
#include "ServerCommon.h"

using namespace boost::property_tree;

using namespace spine::common;
using namespace spine::server;

std::recursive_mutex SpineLevel::_lock;
std::map<int, SendUserLevelMessage> SpineLevel::_levels;
std::vector<SpineLevel::RankingEntry> SpineLevel::_rankings;
std::mutex SpineLevel::_rankingLock;

void SpineLevel::init() {
	std::thread([]() {
		std::map<int, std::string> userList;

		do {
			MariaDBWrapper accountDatabase;
			if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
				break;
			}

			if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT ID, Username FROM accounts\";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!accountDatabase.query("EXECUTE selectStmt;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			const auto results = accountDatabase.getResults<std::vector<std::string>>();

			for (const auto & vec : results) {
				const int id = std::stoi(vec[0]);
				const std::string username = vec[1];

				userList.insert(std::make_pair(id, username));
			}
		} while (false);
		
		for (auto & p : userList) {
			{
				std::lock_guard<std::mutex> lg(_rankingLock);
				
				if (std::find_if(_rankings.begin(), _rankings.end(), [p](const RankingEntry & a) {
						return p.first == a.userID;
					}) != _rankings.end()) continue;
			}
			
			getLevel(p.first);
		}
	}).detach();
}

SendUserLevelMessage SpineLevel::getLevel(int userID) {
	std::lock_guard<std::recursive_mutex> lg(_lock);
	auto it = _levels.find(userID);
	
	if (it != _levels.end()) return it->second;

	cacheLevel(userID);

	it = _levels.find(userID);
	
	return it->second;
}

void SpineLevel::updateLevel(int userID) {
	std::lock_guard<std::recursive_mutex> lg(_lock);
	const auto it = _levels.find(userID);

	if (it != _levels.end()) {
		_levels.erase(it);
	}

	cacheLevel(userID);
}

void SpineLevel::clear(const std::vector<int> & userList) {
	std::lock_guard<std::recursive_mutex> lg(_lock);

	for (int userID : userList) {
		auto it = _levels.find(userID);

		if (it == _levels.end()) continue;
		
		_levels.erase(it);
	}
}

void SpineLevel::addRanking(boost::property_tree::ptree & json) {
	std::lock_guard<std::mutex> lg(_rankingLock);

	std::cout << "Adding Ranking" << std::endl;
	
	std::sort(_rankings.begin(), _rankings.end(), [](const RankingEntry & a, const RankingEntry & b) {
		return a.level > b.level || (a.level == b.level && a.xp > b.xp);
	});

	uint32_t rank = 0;
	uint32_t lastXP = 0;
	uint32_t realRank = 0;

	ptree rankingList;
	
	for (const auto & re : _rankings) {
		realRank++;

		if (re.xp != lastXP) {
			rank = realRank;
			lastXP = re.xp;
		}
		
		ptree rankingEntry;
		
		rankingEntry.put("Name", re.username);
		rankingEntry.put("Level", re.level);
		rankingEntry.put("XP", re.xp);
		rankingEntry.put("Rank", rank);

		rankingList.push_back(std::make_pair("", rankingEntry));
	}
	
	json.add_child("Ranking", rankingList);
}

void SpineLevel::cacheLevel(int userID) {
	SendUserLevelMessage sulm;

	int level = 0;
	uint32_t currentXP = 0;

	do {
		CONNECTTODATABASE(__LINE__);
		
		if (!database.query("PREPARE selectAchievementsStmt FROM \"SELECT COUNT(*) FROM modAchievements WHERE UserID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectAchievementListStmt FROM \"SELECT COUNT(*) FROM modAchievementList WHERE ModID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectAchievementsPerModStmt FROM \"SELECT COUNT(*) FROM modAchievements WHERE UserID = ? AND ModID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectAchievementWithModsStmt FROM \"SELECT DISTINCT ModID FROM modAchievements WHERE UserID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectScoresStmt FROM \"SELECT Count(*) FROM modScores WHERE UserID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectRatingsStmt FROM \"SELECT Count(*) FROM ratings WHERE UserID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectCompatibilitiesStmt FROM \"SELECT Count(*) FROM compatibilityList WHERE UserID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectPlayedModsWithTimeStmt FROM \"SELECT ModID, Duration FROM playtimes WHERE UserID = ?\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("PREPARE selectPlayTimesStmt FROM \"SELECT Duration FROM playtimes WHERE ModID = ? ORDER BY Duration ASC\";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		
		if (!database.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
			std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
			break;
		}
		if (!database.query("EXECUTE selectAchievementsStmt USING @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
			break;
		}
		auto lastResults = database.getResults<std::vector<std::string>>();
		if (!lastResults.empty()) {
			currentXP += std::stoi(lastResults[0][0]) * 50; // 50 EP per achievement
		}

		// perfect games (all achievements)
		if (!database.query("EXECUTE selectAchievementWithModsStmt USING @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
			break;
		}
		lastResults = database.getResults<std::vector<std::string>>();
		for (const auto & vec : lastResults) {
			if (!database.query("SET @paramModID=" + vec[0] + ";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!database.query("EXECUTE selectAchievementListStmt USING @paramModID;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			auto r = database.getResults<std::vector<std::string>>();
			const uint32_t achievementCount = std::stoi(r[0][0]);
			
			if (!database.query("EXECUTE selectAchievementsPerModStmt USING @paramUserID, @paramModID;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			r = database.getResults<std::vector<std::string>>();
			const uint32_t unlockedAchievementCount = std::stoi(r[0][0]);

			if (unlockedAchievementCount == achievementCount) {
				currentXP += 1000; // 1000 EP for perfect games
			}
		}
		
		if (!database.query("EXECUTE selectScoresStmt USING @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
			break;
		}
		lastResults = database.getResults<std::vector<std::string>>();
		if (!lastResults.empty()) {
			currentXP += std::stoi(lastResults[0][0]) * 100; // 100 EP per score
		}
		
		if (!database.query("EXECUTE selectRatingsStmt USING @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
			break;
		}
		lastResults = database.getResults<std::vector<std::string>>();
		if (!lastResults.empty()) {
			currentXP += std::stoi(lastResults[0][0]) * 250; // 250 EP per rating
		}
		
		if (!database.query("EXECUTE selectCompatibilitiesStmt USING @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
			break;
		}
		lastResults = database.getResults<std::vector<std::string>>();
		if (!lastResults.empty()) {
			currentXP += std::stoi(lastResults[0][0]) * 10; // 10 EP per compatibility list entry
		}

		// play time over median or even third quartile gives some bonus XP
		
		if (!database.query("EXECUTE selectPlayedModsWithTimeStmt USING @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
			break;
		}
		lastResults = database.getResults<std::vector<std::string>>();
		for (const auto & vec : lastResults) {
			const int ownDuration = std::stoi(vec[1]);
			if (!database.query("SET @paramModID=" + vec[0] + ";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!database.query("EXECUTE selectPlayTimesStmt USING @paramModID;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			const auto r = database.getResults<std::vector<std::string>>();
			const int firstQuartile = std::stoi(r[r.size() / 4][0]);
			const int median = std::stoi(r[r.size() / 2][0]);
			const int thirdQuartile = std::stoi(r[(r.size() * 3) / 4][0]);

			if (ownDuration > thirdQuartile) {
				currentXP += 250;
			} else if (ownDuration > median) {
				currentXP += 100;
			} else if (ownDuration > firstQuartile) {
				currentXP += 50;
			}
		}

		// bonus XP for people that support us by playing our games
		{
			MariaDBWrapper ewDatabase;
			if (!ewDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, EWDATABASE, 0)) {
				std::cout << "Couldn't connect to database: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			if (!ewDatabase.query("PREPARE selectPlayedTimeStmt FROM \"SELECT Time FROM playTimes WHERE UserID = ? LIMIT 1\";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
		
			if (!ewDatabase.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!ewDatabase.query("EXECUTE selectPlayedTimeStmt USING @paramUserID;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			const auto r = ewDatabase.getResults<std::vector<std::string>>();
			if (!r.empty()) {
				currentXP += 1000;
			}
		}
		{
			MariaDBWrapper tri6Database;
			if (!tri6Database.connect("localhost", DATABASEUSER, DATABASEPASSWORD, TRI6DATABASE, 0)) {
				std::cout << "Couldn't connect to database: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			if (!tri6Database.query("PREPARE selectPlayedTimeStmt FROM \"SELECT Time FROM playTimes WHERE UserID = ? LIMIT 1\";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!tri6Database.query("PREPARE selectPlayedTimeDemoStmt FROM \"SELECT Time FROM playTimesDemo WHERE UserID = ? LIMIT 1\";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
		
			if (!tri6Database.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
				std::cout << "Query couldn't be started: " << __LINE__ << std::endl;
				break;
			}
			if (!tri6Database.query("EXECUTE selectPlayedTimeStmt USING @paramUserID;")) {
				std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
				break;
			}
			auto r = tri6Database.getResults<std::vector<std::string>>();
			if (!r.empty()) {
				currentXP += 1000;
			} else {
				if (!tri6Database.query("EXECUTE selectPlayedTimeDemoStmt USING @paramUserID;")) {
					std::cout << "Query couldn't be started: " << __LINE__ << /*" " << database.getLastError() <<*/ std::endl;
					break;
				}
				r = tri6Database.getResults<std::vector<std::string>>();
				if (!r.empty()) {
					currentXP += 250;
				}
			}
		}
	} while (false);

	uint32_t nextXP = 500;
	while (currentXP >= nextXP) {
		level++;
		nextXP += (level + 1) * 500;
	}
	
	sulm.level = level;
	sulm.currentXP = currentXP;
	sulm.nextXP = nextXP;

	_levels[userID] = sulm;

	std::lock_guard<std::mutex> lg(_rankingLock);
	
	const auto it = std::find_if(_rankings.begin(), _rankings.end(), [userID](const RankingEntry & re) {
		return re.userID == userID;
	});
	
	if (it == _rankings.end()) {
		RankingEntry re;
		re.userID = userID;
		re.xp = currentXP;
		re.level = level;
		re.username = ServerCommon::getUsername(userID);
		
		_rankings.push_back(re);
	} else {
		it->xp = currentXP;
		it->level = level;
	}
}
