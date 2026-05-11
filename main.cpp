#pragma warning(push)
#pragma warning(disable: 4996)
#pragma warning(disable: 4251)
#pragma warning(disable: 4275)
#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <conio.h>
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include "Process.h"

std::map<std::string, Process*> processes;

struct ProcessConfig {
	std::string path;
	std::string args;
	std::string working_dir;
	unsigned int check_interval;
	std::string window_name;

	bool operator==(const ProcessConfig& other) const {
		return path == other.path && 
			args == other.args &&
			working_dir == other.working_dir &&
			window_name == other.window_name;
	}
};

struct State {
	std::atomic_bool need_reload{ false };
} state;

static bool parse_config(const std::string& config_path, std::map<std::string, ProcessConfig>& out)
{
	YAML::Node config = YAML::LoadFile(config_path);
	bool is_valid = true;

	for (auto it = config.begin(); it != config.end(); ++it)
	{
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
		unsigned int check_interval = process_config["checking-interval"].IsDefined() ? process_config["checking-interval"].as<unsigned int>() : 10;
		std::string window_name = process_config["window-name"].IsDefined() ? process_config["window-name"].as<std::string>() : "";

		if (path.empty())
		{
			std::cout << "Path is required for process: " << process_name << std::endl;
			is_valid = false;
			continue;
		}

		out[process_name] = { path, args, working_dir, check_interval, window_name };
	}

	return is_valid;
}

bool load_config(const std::string& config_path)
{
	std::map<std::string, ProcessConfig> new_configs;
	if (!parse_config(config_path, new_configs))
		return false;

	YAML::Node config = YAML::LoadFile(config_path);
	if (config["window-name"].IsDefined())
		SetConsoleTitleA(config["window-name"].as<std::string>().c_str());

	// Remove processes that are no longer in the config
	for (auto it = processes.begin(); it != processes.end(); )
	{
		if (new_configs.find(it->first) == new_configs.end())
		{
			delete it->second;
			it = processes.erase(it);
		}
		else
		{
			++it;
		}
	}

	// Add new or restart changed processes
	for (auto& [name, cfg] : new_configs)
	{
		auto it = processes.find(name);
		if (it != processes.end())
		{
			Process* p = it->second;
			ProcessConfig existing_cfg = { p->get_path(), p->get_args(), p->get_working_dir(), p->get_check_interval(), p->get_window_name() };
			if (!(existing_cfg == cfg))
			{
				// Config changed: stop and replace the process
				if (p->is_running())
				{
					if (!p->stop())
						std::cout << "Failed to stop process: " << name << std::endl;
				}
				delete p;
				it->second = new Process(name, cfg.path, cfg.args, cfg.working_dir, cfg.check_interval, cfg.window_name);
			}
			// else: unchanged, keep as-is
		}
		else
		{
			processes[name] = new Process(name, cfg.path, cfg.args, cfg.working_dir, cfg.check_interval, cfg.window_name);
		}
	}

	return true;
}

void keyboard_listener(const std::string& config_path) {
	while (true)
	{
		if (_kbhit())
		{
			int ch = _getch();
			if (ch == 'r' || ch == 'R')
			{
				state.need_reload = true;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

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

	if (!load_config(argv[1]))
	{
		system("pause");
		return 1;
	}

	std::thread(keyboard_listener, std::string(argv[1])).detach();

	while (true)
	{
		if (state.need_reload)
		{
			if (!load_config(argv[1]))
			{
				std::cout << "Failed to reload config." << std::endl;
				state.need_reload = false;
				std::this_thread::sleep_for(std::chrono::seconds(2));
				continue;
			}
			state.need_reload = false;
		}

		system("cls");
		printf("Simple Process Manager\n");
		printf("Press R to reload config\n\n");
		unsigned int i = 1;
		time_t now = time(0);
		for (auto& [name, process] : processes)
		{
			bool is_running = process->is_running();
			std::string time_elapsed = "";
			if (is_running) {
				unsigned int seconds = 0;
				unsigned int minutes = 0;
				unsigned int hours = 0;
				unsigned int days = 0;

				div_t minuteAndSecond = div((int)(now - process->get_start_time()), (int)60);
				minutes = minuteAndSecond.quot;
				seconds = minuteAndSecond.rem;
				div_t hourAndMinute = div((int)minutes, (int)60);
				hours = hourAndMinute.quot;
				minutes = hourAndMinute.rem;
				div_t dayAndHour = div((int)hours, (int)24);
				days = dayAndHour.quot;
				hours = dayAndHour.rem;

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

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}