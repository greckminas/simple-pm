#pragma once

#include <string>
#include <time.h>
#include <Windows.h>

class Process
{
private:
	static HANDLE hProcessJob;
	HANDLE process_handle;
	std::string name;
	std::string path;
	std::string args;
	std::string working_dir;

	time_t start_time;
	time_t last_checked_time;
	unsigned int check_interval; // seconds

	void set_last_checked_time(time_t last_checked_time);
	void set_start_time(time_t start_time);
	void set_check_interval(unsigned int check_interval);

public:
	static void Init();

	Process(std::string name, std::string path, std::string args, std::string working_dir, unsigned int check_interval);
	~Process();

	std::string get_name();
	std::string get_path();
	std::string get_args();
	std::string get_working_dir();
	time_t get_start_time();
	time_t get_last_checked_time();
	unsigned int get_check_interval();

	bool is_running();
	bool is_time_to_check();
	bool start();
	bool stop();
	void execute();
};