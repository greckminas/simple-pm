#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Process.h"

std::vector<Process*> processes;

int main(int argc, char *argv[])
{
	// usage:
	// .\simple-pm.exe /path/to/config.yaml
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " /path/to/config.yaml" << std::endl;
		return 1;
	}

	Process::Init();

	// open config file
	YAML::Node config = YAML::LoadFile(argv[1]);
	bool is_valid = true;

	for (auto it = config.begin(); it != config.end(); ++it)
	{
		// key = process name
		// value = process config
		std::string process_name = it->first.as<std::string>();
		YAML::Node process_config = it->second;

		if (!process_config.IsMap())
		{
			std::cout << "Invalid config for process: " << process_name << std::endl;
			is_valid = false;
			continue;
		}
		
		std::string path = process_config["path"].IsDefined() ? process_config["path"].as<std::string>() : "";
		std::string args = process_config["args"].IsDefined() ? process_config["args"].as<std::string>() : "";
		std::string command = process_config["command"].IsDefined() ? process_config["command"].as<std::string>() : "";

		if (!command.empty()) {
			if (!path.empty() || !args.empty()) {
				throw std::runtime_error("Cannot specify both 'command' and 'path'/'args' for process: " + process_name);
			}
			path = "cmd.exe";
			args = "/c " + command;
		}

		std::string working_dir = process_config["working-dir"].IsDefined() ? process_config["working-dir"].as<std::string>() : "";
		unsigned int check_interval = process_config["check-interval"].IsDefined() ? process_config["check-interval"].as<unsigned int>() : 10;

		if (path.empty())
		{
			std::cout << "Path is required for process: " << process_name << std::endl;
			is_valid = false;
			continue;
		}

		Process *process = new Process(process_name, path, args, working_dir, check_interval);
		processes.push_back(process);
	}

	if (!is_valid)
	{
		system("pause");
		return 1;
	}

	while (true)
	{
		system("cls");
		printf("Simple Process Manager\n\n");
		unsigned int i = 1;
		time_t now = time(0);
		for (Process* process : processes)
		{
			bool is_running = process->is_running();
			std::string time_elapsed = "";
			if (is_running) {
				unsigned int seconds = 0;
				unsigned int minutes = 0;
				unsigned int hours = 0;
				unsigned int days = 0;

				div_t minuteAndSecond = div((int)(now - process->get_start_time()), (int)60);
				seconds = minuteAndSecond.rem;
				minutes = minuteAndSecond.quot;
				div_t hourAndMinute = div((int)minutes, (int)60);
				minutes = hourAndMinute.rem;
				hours = hourAndMinute.quot;
				div_t dayAndHour = div((int)hours, (int)24);
				hours = dayAndHour.rem;
				days = dayAndHour.quot;

				if (days > 0) {
					time_elapsed += std::to_string(days) + " days ";
				}
				if (hours > 0) {
					time_elapsed += std::to_string(hours) + " hours ";
				}
				if (minutes > 0) {
					time_elapsed += std::to_string(minutes) + " minutes ";
				}
				if (seconds > 0) {
					time_elapsed += std::to_string(seconds) + " seconds ";
				}
				time_elapsed += "ago";
			}

			printf("%d. %s (%s) (%s)\n", 
				i++, 
				process->get_name().c_str(), 
				is_running ? "Running" : "Stopped",
				time_elapsed.c_str()
				);
			bool is_time_to_check = process->is_time_to_check();
			if (is_time_to_check)
			{
				process->execute();
			}
		}

		Sleep(1000);
	}

	return 0;
}