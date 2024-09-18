#include "Process.h"

HANDLE Process::hProcessJob = NULL;

void Process::Init()
{
	if (Process::hProcessJob != NULL)
	{
		CloseHandle(Process::hProcessJob);
		Process::hProcessJob = NULL;
	}

	Process::hProcessJob = CreateJobObjectA(NULL, NULL);

	if (Process::hProcessJob == NULL)
	{
		throw std::exception("Failed to create job object");
	}

	JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
	jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	SetInformationJobObject(Process::hProcessJob, JOBOBJECTINFOCLASS::JobObjectExtendedLimitInformation, &jeli, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));

	if (AssignProcessToJobObject(Process::hProcessJob, GetCurrentProcess()) == FALSE) {
		DWORD Error = GetLastError();
	}
}

Process::Process(std::string name, std::string path, std::string args, std::string working_dir, unsigned int check_interval)
{
	if (Process::hProcessJob == NULL)
	{
		throw std::exception("Process is not initialized");
	}

	this->process_handle = NULL;
	this->start_time = 0;
	this->last_checked_time = 0;

	this->name = name;
	this->path = path;
	this->args = args;
	this->working_dir = working_dir;
	this->check_interval = check_interval;
}

Process::~Process()
{
	if (this->process_handle != NULL)
	{
		this->stop();
	}
}

std::string Process::get_name()
{
	return this->name;
}

std::string Process::get_path()
{
	return this->path;
}

std::string Process::get_args()
{
	return this->args;
}

std::string Process::get_working_dir()
{
	return this->working_dir;
}

time_t Process::get_start_time()
{
	return this->start_time;
}

time_t Process::get_last_checked_time()
{
	return this->last_checked_time;
}

unsigned int Process::get_check_interval()
{
	return this->check_interval;
}

void Process::set_last_checked_time(time_t last_checked_time)
{
	this->last_checked_time = last_checked_time;
}

void Process::set_start_time(time_t start_time)
{
	this->start_time = start_time;
}

void Process::set_check_interval(unsigned int check_interval)
{
	this->check_interval = check_interval;
}

bool Process::is_running()
{
	if (this->process_handle == NULL)
	{
		return false;
	}

	DWORD exit_code;
	GetExitCodeProcess(this->process_handle, &exit_code);

	return exit_code == STILL_ACTIVE;
}

bool Process::is_time_to_check()
{
	time_t current_time = time(NULL);

	bool result = current_time - this->last_checked_time >= this->check_interval;
	if (result) {
		this->set_last_checked_time(current_time);
	}
	return result;
}

bool Process::start()
{
	if (this->is_running())
	{
		return false;
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	std::string command = this->path + " " + this->args;

	if (!CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, this->working_dir.empty() ? NULL : this->working_dir.c_str(), &si, &pi))
	{
		this->set_last_checked_time(time(NULL));
		return false;
	}
	
	CloseHandle(pi.hThread);

	this->process_handle = pi.hProcess;
	this->set_start_time(time(NULL));
	this->set_last_checked_time(time(NULL));

	return true;
}

bool Process::stop()
{
	if (this->process_handle == NULL)
	{
		return false;
	}

	TerminateProcess(this->process_handle, 0);
	CloseHandle(this->process_handle);
	this->process_handle = NULL;

	return true;
}

void Process::execute()
{
	if (this->is_running())
	{
		return;
	}

	this->start();
}