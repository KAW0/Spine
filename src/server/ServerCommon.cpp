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
// Copyright 2019 Clockwork Origins

#include "ServerCommon.h"

#include <iostream>
#include <map>
#include <regex>
#include <thread>

#include "LanguageConverter.h"
#include "MariaDBWrapper.h"
#include "SpineServerConfig.h"
#include "Smtp.h"

#include "common/Language.h"

using namespace spine::common;
using namespace spine::server;

std::string ServerCommon::convertString(const std::string & str) {
	std::string result = str;
	result = std::regex_replace(result, std::regex("%20"), " ");
	result = std::regex_replace(result, std::regex("%21"), "!");
	result = std::regex_replace(result, std::regex("%22"), "\"");
	result = std::regex_replace(result, std::regex("%23"), "#");
	result = std::regex_replace(result, std::regex("%24"), "$");
	result = std::regex_replace(result, std::regex("%25"), "%");
	result = std::regex_replace(result, std::regex("%26"), "&");
	result = std::regex_replace(result, std::regex("%27"), "'");
	result = std::regex_replace(result, std::regex("%28"), "(");
	result = std::regex_replace(result, std::regex("%29"), ")");
	result = std::regex_replace(result, std::regex("%2a"), "*");
	result = std::regex_replace(result, std::regex("%2b"), "+");
	result = std::regex_replace(result, std::regex("%2c"), ",");
	result = std::regex_replace(result, std::regex("%2d"), "-");
	result = std::regex_replace(result, std::regex("%2e"), ".");
	result = std::regex_replace(result, std::regex("%2f"), "/");
	result = std::regex_replace(result, std::regex("%3a"), ":");
	result = std::regex_replace(result, std::regex("%3b"), ";");
	result = std::regex_replace(result, std::regex("%3c"), "<");
	result = std::regex_replace(result, std::regex("%3d"), "=");
	result = std::regex_replace(result, std::regex("%3e"), ">");
	result = std::regex_replace(result, std::regex("%3f"), "?");
	result = std::regex_replace(result, std::regex("%40"), "@");
	result = std::regex_replace(result, std::regex("%5b"), "[");
	result = std::regex_replace(result, std::regex("%5c"), "\\");
	result = std::regex_replace(result, std::regex("%5d"), "]");
	result = std::regex_replace(result, std::regex("%5e"), "^");
	result = std::regex_replace(result, std::regex("%5f"), "_");
	result = std::regex_replace(result, std::regex("%7b"), "{");
	result = std::regex_replace(result, std::regex("%7d"), "}");
	return result;
}

int ServerCommon::getUserID(const std::string & username, const std::string & password) {
	MariaDBWrapper accountDatabase;
	if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
		std::cout << "Couldn't connect to database" << std::endl;
		return -1;
	}

	if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT ID FROM accounts WHERE Username = ? AND Password = PASSWORD(?) LIMIT 1\";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return -1;
	}
	if (!accountDatabase.query("SET @paramUsername='" + username + "';")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return -1;
	}
	if (!accountDatabase.query("SET @paramPassword='" + password + "';")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return -1;
	}
	if (!accountDatabase.query("EXECUTE selectStmt USING @paramUsername, @paramPassword;")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
	}
	const auto results = accountDatabase.getResults<std::vector<std::string>>();

	return results.empty() ? -1 : std::stoi(results[0][0]);
}

int ServerCommon::getUserID(const std::string & username) {
	MariaDBWrapper accountDatabase;
	if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
		std::cout << "Couldn't connect to database" << std::endl;
		return -1;
	}

	if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT ID FROM accounts WHERE Username = ? LIMIT 1\";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return -1;
	}
	if (!accountDatabase.query("SET @paramUsername='" + username + "';")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return -1;
	}
	if (!accountDatabase.query("EXECUTE selectStmt USING @paramUsername;")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
	}
	const auto results = accountDatabase.getResults<std::vector<std::string>>();

	return results.empty() ? -1 : std::stoi(results[0][0]);
}

std::string ServerCommon::getUsername(const int id) {
	MariaDBWrapper accountDatabase;
	if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
		return "";
	}

	if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT Username FROM accounts WHERE ID = ? LIMIT 1\";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return "";
	}
	if (!accountDatabase.query("SET @paramID=" + std::to_string(id) + ";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return "";
	}
	if (!accountDatabase.query("EXECUTE selectStmt USING @paramID;")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
	}
	const auto results = accountDatabase.getResults<std::vector<std::string>>();

	return results.empty() ? "" : filterUsername(results[0][0]);
}

std::vector<std::string> ServerCommon::getUserList() {
	MariaDBWrapper accountDatabase;
	if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
		return {};
	}

	if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT ID, Username FROM accounts ORDER BY Username ASC\";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return {};
	}
	if (!accountDatabase.query("EXECUTE selectStmt;")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return {};
	}
	auto results = accountDatabase.getResults<std::vector<std::string>>();

	std::map<std::string, std::string> userMap;
	
	for (const auto & vec : results) {
		userMap.insert(std::make_pair(vec[0], filterUsername(vec[1])));
	}

	std::vector<std::string> users;
	do {
		CONNECTTODATABASE(__LINE__)

		if (!database.query("PREPARE selectStmt FROM \"SELECT UserID FROM lastLoginTimes\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("EXECUTE selectStmt;")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		results = database.getResults<std::vector<std::string>>();
		
		users.reserve(results.size());

		for (const auto & vec : results) {
			const auto it = userMap.find(vec[0]);

			if (it == userMap.end()) continue;
			
			users.push_back(it->second);
		}
	} while (false);
	
	return users;
}

void ServerCommon::sendMail(const std::string & subject, const std::string & body, const std::string & replyTo) {
	sendMail(subject, body, replyTo, "bonne@clockwork-origins.de");
}

void ServerCommon::sendMail(const std::string & subject, const std::string & body, const std::string & replyTo, const std::string & receiver) {
	std::thread([subject, body, replyTo, receiver]() {
		const Smtp s("127.0.0.1");
		const bool b = s.sendMail("contact@clockwork-origins.de", receiver, subject, body, replyTo);
		static_cast<void>(b);
	}).detach();
}

bool ServerCommon::isValidUserID(int userID) {
	MariaDBWrapper accountDatabase;
	if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
		return false;
	}

	if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT ID FROM accounts WHERE ID = ? LIMIT 1\";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return false;
	}
	if (!accountDatabase.query("SET @paramID=" + std::to_string(userID) + ";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return false;
	}
	if (!accountDatabase.query("EXECUTE selectStmt USING @paramID;")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
	}
	const auto results = accountDatabase.getResults<std::vector<std::string>>();

	return !results.empty();
}

std::string ServerCommon::getProjectName(int projectID, int preferredLanguage) {
	do {
		CONNECTTODATABASE(__LINE__)
		
		if (!database.query("PREPARE selectProjectNameStmt FROM \"SELECT CAST(Name AS BINARY) FROM projectNames WHERE ProjectID = ? AND Languages & ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("PREPARE selectFallbackProjectNameStmt FROM \"SELECT CAST(Name AS BINARY) FROM projectNames WHERE ProjectID = ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramProjectID=" + std::to_string(projectID) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramLanguage=" + std::to_string(preferredLanguage) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramEnglishLanguage=" + std::to_string(English) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("EXECUTE selectProjectNameStmt USING @paramProjectID, @paramLanguage;")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
			break;
		}
		auto results = database.getResults<std::vector<std::string>>();
		if (results.empty()) {
			if (!database.query("EXECUTE selectProjectNameStmt USING @paramProjectID, @paramEnglishLanguage;")) {
				std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
				break;
			}
			results = database.getResults<std::vector<std::string>>();
			
			if (results.empty()) {
				if (!database.query("EXECUTE selectFallbackProjectNameStmt USING @paramProjectID;")) {
					std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
					break;
				}
				results = database.getResults<std::vector<std::string>>();
				
				if (results.empty()) break;
			}
		}

		return results[0][0];
	} while (false);

	return "";
}

std::string ServerCommon::getPackageName(int packageID, int preferredLanguage) {
	do {
		CONNECTTODATABASE(__LINE__)

		if (!database.query("PREPARE selectPackageNameStmt FROM \"SELECT CAST(Name AS BINARY) FROM optionalpackagenames WHERE PackageID = ? AND Language = ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("PREPARE selectFallbackPackageNameStmt FROM \"SELECT CAST(Name AS BINARY) FROM optionalpackagenames WHERE PackageID = ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramPackageID=" + std::to_string(packageID) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramLanguage='" + LanguageConverter::convert(static_cast<Language>(preferredLanguage)) + "';")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramEnglishLanguage='" + LanguageConverter::convert(English) + "';")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("EXECUTE selectPackageNameStmt USING @paramPackageID, @paramLanguage;")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
			break;
		}
		auto results = database.getResults<std::vector<std::string>>();
		if (results.empty()) {
			if (!database.query("EXECUTE selectPackageNameStmt USING @paramPackageID, @paramEnglishLanguage;")) {
				std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
				break;
			}
			results = database.getResults<std::vector<std::string>>();

			if (results.empty()) {
				if (!database.query("EXECUTE selectFallbackPackageNameStmt USING @paramPackageID;")) {
					std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
					break;
				}
				results = database.getResults<std::vector<std::string>>();

				if (results.empty()) break;
			}
		}

		return results[0][0];
	} while (false);

	return "";
}

std::string ServerCommon::filterUsername(const std::string & username) {
	MariaDBWrapper accountDatabase;
	if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
		return username;
	}

	if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT Filter FROM textFilters\";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return username;
	}
	if (!accountDatabase.query("EXECUTE selectStmt;")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return username;
	}
	const auto results = accountDatabase.getResults<std::vector<std::string>>();

	auto result = username;

	for (const auto & vec : results) {
		result = std::regex_replace(result, std::regex(vec[0]), "******");
	}
	
	return result;
}

bool ServerCommon::hasPrivilege(int userID, UserPrivilege privilege) {
	do {
		CONNECTTODATABASE(__LINE__)

		if (!database.query("PREPARE selectPrivilegeStmt FROM \"SELECT UserID FROM userPrivileges WHERE UserID = ? AND Privileges & ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramPrivilege=" + std::to_string(static_cast<int>(privilege)) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("EXECUTE selectPrivilegeStmt USING @paramUserID, @paramPrivilege;")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
			break;
		}
		const auto results = database.getResults<std::vector<std::string>>();
		return !results.empty();
	} while (false);

	return false;
}

bool ServerCommon::canAccessProject(int userID, int projectID) {
	do {
		CONNECTTODATABASE(__LINE__)

		if (!database.query("PREPARE selectProjectStmt FROM \"SELECT TeamID, Enabled FROM mods WHERE ModID = ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("PREPARE selectMemberStmt FROM \"SELECT UserID FROM teammembers WHERE TeamID = ? AND UserID = ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("PREPARE selectEAStmt FROM \"SELECT UserID FROM earlyUnlocks WHERE ModID = ? AND UserID = ? LIMIT 1\";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramProjectID=" + std::to_string(projectID) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("EXECUTE selectProjectStmt USING @paramProjectID;")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
			break;
		}
		auto results = database.getResults<std::vector<std::string>>();

		if (results.empty()) return false;

		const auto vec = results[0];

		if (vec[1] == "1") return true;

		if (!database.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("SET @paramTeamID=" + vec[0] + ";")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << database.getLastError() << std::endl;
			break;
		}
		if (!database.query("EXECUTE selectMemberStmt USING @paramTeamID, @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
			break;
		}
		results = database.getResults<std::vector<std::string>>();

		if (!results.empty()) return true;
		
		if (!database.query("EXECUTE selectEAStmt USING @paramProjectID, @paramUserID;")) {
			std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << " " << database.getLastError() << std::endl;
			break;
		}
		results = database.getResults<std::vector<std::string>>();

		return !results.empty();
	} while (false);

	return false;
}

int ServerCommon::getPatronLevel(int userID) {
	MariaDBWrapper accountDatabase;
	if (!accountDatabase.connect("localhost", DATABASEUSER, DATABASEPASSWORD, ACCOUNTSDATABASE, 0)) {
		return 0;
	}

	if (!accountDatabase.query("PREPARE selectStmt FROM \"SELECT Level FROM patronLevels WHERE ID = ? AND ProjectID = 0 LIMIT 1\";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return 0;
	}
	if (!accountDatabase.query("SET @paramUserID=" + std::to_string(userID) + ";")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return 0;
	}
	if (!accountDatabase.query("EXECUTE selectStmt USING @paramUserID;")) {
		std::cout << "Query couldn't be started: " << __FILE__ << ": " << __LINE__ << ": " << accountDatabase.getLastError() << std::endl;
		return 0;
	}
	const auto results = accountDatabase.getResults<std::vector<std::string>>();

	if (results.empty()) return 0;

	return std::stoi(results[0][0]);
}
