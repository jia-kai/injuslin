/*
 * $File: judge_core.cpp
 * $Author: Jiakai <jia.kai66@gmail.com>
 * $Date: Sat Dec 22 11:14:19 2012 +0800
 */
/*
Copyright (C) (2008, 2009) (jiakai) <jia.kai66@gmail.com>

This program is free software; you can redistribute it and/or 
modify it under the terms of the GNU General Public License 
as published by the Free Software Foundation; either 
version 2 of the License, or any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#define MAX_COMPILATION_TIME			10
#define COMPILATION_SYS_ERROR_EXIT_CODE	12

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/select.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <cstring>
using namespace std;

#include "judge_core.h"
#include "common.h"

#define BUFFER_SIZE	16384
#define PROG_NAME	"prog"

//----------------------Function Declaration----------------------------------
static bool execute_program(const Test_point &point_conf, Judge_result_point &result);
static bool remove_dir(const char *path);
static bool copy_file(const char *src, const char *dest);
//  For the three functions above:
	//  Return		:
	//		On success, true is returned. 
	//		If a system error occurs, return false and the global variable @global_error_msg in common.h is set.
static bool compile(const char *source_file, const char *command, string &compiler_output);
	// return: false if there is an system error, and global_error_msg is set
static bool do_compile(const char *command, string &compiler_output);
	// return: false if there is an system error, and global_error_msg is set
#ifdef MAX_COMPILATION_TIME
static bool compile_time_limit_error;
static void* compile_time_limit_thread(void *pgid);
	// compile_time_limit_error is set true if an system error occurred, and global_error_msg is set
#endif
//----------------------------------------------------------------------------

#define INIT_ERROR_MSG(_err_) \
	snprintf(global_error_msg, CSTRING_MAX_LEN, "%s\n  %s\n  (At %s : %s : %d)",  (_err_), \
			strerror(errno), __FILE__, __PRETTY_FUNCTION__, __LINE__);

bool judge_core(const Contest_config &ct_conf, const Problem_config &prob_conf, const string &contestant_name,
		File_compare fc, void *fc_user_data, Judge_result_problem &result, Judge_core_cb cb_result_updated, void *cb_user_data)
{
#define CB_PROGRESS(_arg0_, _arg1_) \
	do \
	{ \
		if (cb_result_updated != NULL) \
		{ \
			if (cb_result_updated((_arg0_), (_arg1_), cb_user_data)) \
				return true; \
		} \
	} while(0)

	remove_dir(TEMP_DIRECTORY);

	if (mkdir(TEMP_DIRECTORY, 0777))
	{
		INIT_ERROR_MSG("mkdir");
		return false;
	}
	if (chmod(TEMP_DIRECTORY, 0777))
	{
		INIT_ERROR_MSG("chmod");
		return false;
	}

	result.source_found = false;
	string src_file_path(ct_conf.source_dir);
	src_file_path.append(contestant_name).append("/").append(prob_conf.source).append(".");
	for (Str_map::const_iterator i = ct_conf.compiler.begin(); i != ct_conf.compiler.end(); i ++)
	{
		string::size_type len = src_file_path.length();
		src_file_path.append(i->first);

		if (access(src_file_path.c_str(), F_OK | R_OK) == 0)
		{
			result.source_found = true;

			CB_PROGRESS(JUDGE_CORE_CB_SRC_FOUND, (size_t)src_file_path.c_str());

			string tmp(TEMP_DIRECTORY);
			tmp.append(PROG_NAME);
			string::size_type len0 = tmp.length();
			tmp.append(".").append(i->first);
			if (!copy_file(src_file_path.c_str(), tmp.c_str()))
				return false;
			src_file_path = tmp;
			src_file_path.erase(len0);

			CB_PROGRESS(JUDGE_CORE_CB_START_COMPILE, 0);

			if (!compile(src_file_path.c_str(), i->second.c_str(), result.compiler_output))
				return false;
			if (!(result.compile_successful = result.compiler_output.empty()))
			{
				CB_PROGRESS(JUDGE_CORE_CB_COMPILE_FAIL, 0);
				return true;
			}

			CB_PROGRESS(JUDGE_CORE_CB_COMPILE_SUCCESS, 0);
			break;
		}
		src_file_path.erase(len);
	}
	if (!result.source_found)
	{
		CB_PROGRESS(JUDGE_CORE_CB_SRC_NOT_FOUND, 0);
		return true;
	}

	string user_input(TEMP_DIRECTORY), user_output(TEMP_DIRECTORY);
	user_input.append(prob_conf.input);
	user_output.append(prob_conf.output);

	result.points.resize(prob_conf.points.size());

	for (Str_array::size_type i = 0; i != result.points.size(); i ++)
	{
		CB_PROGRESS(JUDGE_CORE_CB_START_EXECUTE, i);
		unlink(user_output.c_str());
		if (!copy_file(prob_conf.points[i].input.c_str(), user_input.c_str()))
			return false;
		if (!execute_program(prob_conf.points[i], result.points[i]))
			return false;

		CB_PROGRESS(JUDGE_CORE_CB_FINISH_EXECUTE, i);
		if (result.points[i].exe_status != Judge_result_point::EXE_NORMAL)
			result.points[i].score = 0;
		else
		{
			CB_PROGRESS(JUDGE_CORE_CB_START_CHECK, i);
			result.points[i].score = fc(prob_conf.points[i].score, prob_conf.points[i].input.c_str(),
					prob_conf.points[i].output.c_str(), user_output.c_str(), fc_user_data);
		}
		CB_PROGRESS(JUDGE_CORE_CB_FINISH_ALL, i);
	}

	if (!remove_dir(TEMP_DIRECTORY))
		return false;

	return true;

#undef CB_PROGRESS
}

bool compile(const char *source_file, const char *command_orig, std::string &compiler_output)
{
	string command;

	for (const char *ptr = command_orig; *ptr; ptr ++)
	{
		if (*ptr == '%' && *(ptr + 1) == 's')
		{
			command.append(source_file);
			ptr ++;
		}else command.append(1, *ptr);
	}

	if (!do_compile(command.c_str(), compiler_output))
		return false;

	return true;
}

bool do_compile(const char *command, string &compiler_output)
{
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1)
	{
		INIT_ERROR_MSG("pipe");
		return false;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		INIT_ERROR_MSG("fork");
		return false;
	}
	else if (pid == 0)
	{
		close(pipe_fd[0]);

		if (dup2(pipe_fd[1], STDOUT_FILENO) == -1 || dup2(pipe_fd[1], STDERR_FILENO) == -1)
		{
			INIT_ERROR_MSG("dup2");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(COMPILATION_SYS_ERROR_EXIT_CODE);
		}

#ifdef MAX_COMPILATION_TIME
		if (setpgrp())
		{
			INIT_ERROR_MSG("setpgrp");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(COMPILATION_SYS_ERROR_EXIT_CODE);
		}
#endif

		execlp("/bin/sh", "sh", "-c", command, NULL);
		char tmp[CSTRING_MAX_LEN];
		snprintf(tmp, CSTRING_MAX_LEN, "execlp(\"/bin/sh\", \"sh\", \"-c\", \"%s\", NULL)", command);
		INIT_ERROR_MSG(tmp);
		write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
		_exit(COMPILATION_SYS_ERROR_EXIT_CODE);
	} else
	{
		close(pipe_fd[1]);

#ifdef MAX_COMPILATION_TIME
		pthread_t thread;
		if (pthread_create(&thread, NULL, compile_time_limit_thread, &pid))
		{
			killpg(pid, SIGKILL);
			INIT_ERROR_MSG("pthread_create");
			return false;
		}
#endif

		compiler_output.clear();
		ssize_t nread;
		while ((nread = read(pipe_fd[0], global_error_msg, CSTRING_MAX_LEN - 1)) > 0)
		{
			global_error_msg[nread] = 0;
			compiler_output.append(global_error_msg);
		}
		close(pipe_fd[0]);

#ifdef MAX_COMPILATION_TIME
		pthread_cancel(thread);
		if (compile_time_limit_error)
			return false;
#endif

		int status;
		if (wait(&status) == -1)
		{
			INIT_ERROR_MSG("wait");
			return false;
		}

		if (WIFSIGNALED(status))
		   	if (WTERMSIG(status) == SIGXCPU || WTERMSIG(status) == SIGKILL)
				compiler_output.assign(_("Compilation time out. Please refer to 'man 1 injuslin' for more information."));

		if (WIFEXITED(status))
		{
			if (WEXITSTATUS(status) == COMPILATION_SYS_ERROR_EXIT_CODE)
			{
				compiler_output.clear();
				return false;
			} else if (WEXITSTATUS(status) == 0)
				compiler_output.clear();
		}

		return true;
	}
}

#ifdef MAX_COMPILATION_TIME
void* compile_time_limit_thread(void *pgid)
{
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	compile_time_limit_error = false;
	timeval tv;
	tv.tv_sec = MAX_COMPILATION_TIME;
	tv.tv_usec = 0;
	if (select(0, NULL, NULL, NULL, &tv) == -1)
	{
		INIT_ERROR_MSG("select");
		compile_time_limit_error = true;
		return NULL;
	}
	if (killpg(*static_cast<pid_t*>(pgid), SIGKILL))
	{
		INIT_ERROR_MSG("killpg");
		compile_time_limit_error = true;
		return NULL;
	}
	return NULL;
}
#endif

bool execute_program(const Test_point &point_conf, Judge_result_point &result)
{
	int pipe_fd[2];
	if (pipe(pipe_fd) == -1)
	{
		INIT_ERROR_MSG("pipe");
		return false;
	}

	pid_t pid = fork();
	if (pid < 0)
	{
		INIT_ERROR_MSG("fork");
		return false;
	}
	else if (pid == 0)
	{
		close(pipe_fd[0]);

		setsid();
		stdin = freopen("/dev/null", "r", stdin);
		stdout = freopen("/dev/null", "w", stdout);
		stderr = freopen("/dev/null", "w", stderr);

		passwd *pd = getpwnam(EXECUTE_USER);
		if (pd == NULL)
		{
			INIT_ERROR_MSG("getpwnam");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(-1);
		}
#ifdef EXECUTE_WITH_CHROOT
		if (chroot(EXECUTE_ROOT_DIR))
		{
			INIT_ERROR_MSG("chroot");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(-1);
		}
		if(chdir(EXECUTE_CHDIR))
#else
		if(chdir(TEMP_DIRECTORY))
#endif
		{
			INIT_ERROR_MSG("chdir");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(-1);
		}
		if (setresgid(pd->pw_gid, pd->pw_gid, pd->pw_gid))
		{
			INIT_ERROR_MSG("setresgid");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(-1);
		}
		if (setresuid(pd->pw_uid, pd->pw_uid, pd->pw_uid))
		{
			INIT_ERROR_MSG("setresuid");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(-1);
		}
		struct rlimit limit;
		limit.rlim_cur = limit.rlim_max = 0;
		if (setrlimit(RLIMIT_NPROC, &limit))
		{
			INIT_ERROR_MSG("setrlimit");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(-1);
		}
#define SET_RLIMIT(_r_, _x_) \
		if (point_conf._r_ != -1) \
		{ \
			limit.rlim_cur = limit.rlim_max = point_conf._r_; \
			if (setrlimit(_x_, &limit) != 0) \
			{ \
				INIT_ERROR_MSG("setrlimit"); \
				write(pipe_fd[1], global_error_msg, strlen(global_error_msg)); \
				_exit(-1); \
			} \
		}
		SET_RLIMIT(rlimit_as, RLIMIT_AS);
		SET_RLIMIT(rlimit_data, RLIMIT_DATA);
		SET_RLIMIT(rlimit_stack, RLIMIT_STACK);
#undef SET_RLIMIT
		limit.rlim_cur = limit.rlim_max = point_conf.time;
		if (setrlimit(RLIMIT_CPU, &limit))
		{
			INIT_ERROR_MSG("setrlimit");
			write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
			_exit(-1);
		}
		execlp("./" PROG_NAME, "./" PROG_NAME, NULL);
		INIT_ERROR_MSG("execlp");
		write(pipe_fd[1], global_error_msg, strlen(global_error_msg));
		_exit(-1);
	}
	else
	{
		close(pipe_fd[1]);
		int status;
		rusage ru;
		if (wait4(pid, &status, 0, &ru) < 0)
		{
			INIT_ERROR_MSG("wait4");
			return false;
		}
		ssize_t global_error_msg_len;
		if ((global_error_msg_len = read(pipe_fd[0], global_error_msg, CSTRING_MAX_LEN)) > 0)
		{
			global_error_msg[global_error_msg_len] = 0;
			close(pipe_fd[0]);
			return false;
		}
		close(pipe_fd[0]);
		result.time = ru.ru_utime.tv_sec * 1000000 + ru.ru_utime.tv_usec + 
					ru.ru_stime.tv_sec * 1000000 + ru.ru_stime.tv_usec;
		result.memory = ((unsigned long long)ru.ru_minflt * 
				(unsigned long long)sysconf(_SC_PAGESIZE)) / 1024ull;
		if (WIFSIGNALED(status))
		{
		   	if (WTERMSIG(status) == SIGXCPU || WTERMSIG(status) == SIGKILL)
				result.exe_status = Judge_result_point::EXE_TIMEOUT;
			if (WTERMSIG(status) == SIGSEGV || (int)result.time  < point_conf.time * 100000) // (real time) < 0.1 * (time limit)
				result.exe_status = Judge_result_point::EXE_MEMORYOUT;
// FIX ME:
//   This is an unpleasant and inaccurate way to judge whether the program exceeds the memory limit.
//   But at present I do not have a better solution.

		}else if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
			result.exe_status = Judge_result_point::EXE_EXIT_NONZERO;
		else result.exe_status = Judge_result_point::EXE_NORMAL;
		return true;
	}
}

bool copy_file(const char *src_path, const char *dest_path)
{
	FILE *src = fopen(src_path, "rb");
	if (src == NULL)
	{
		char tmp[CSTRING_MAX_LEN];
		snprintf(tmp, CSTRING_MAX_LEN, "Copy file from %s to %s: fopen", src_path, dest_path);
		INIT_ERROR_MSG(tmp);
		return false;
	}
	FILE *dest = fopen(dest_path, "wb");
	if (dest == NULL)
	{
		fclose(src);
		char tmp[CSTRING_MAX_LEN];
		snprintf(tmp, CSTRING_MAX_LEN, "Copy file from %s to %s: fopen", src_path, dest_path);
		INIT_ERROR_MSG(tmp);
		return false;
	}
	char buffer[BUFFER_SIZE];
	size_t nchars;
	while ((nchars = fread(buffer, sizeof(char), BUFFER_SIZE, src)) > 0)
		if (fwrite(buffer, sizeof(char), nchars, dest) != nchars)
		{
			fclose(src);
			fclose(dest);
			unlink(dest_path);
			char tmp[CSTRING_MAX_LEN];
			snprintf(tmp, CSTRING_MAX_LEN, "Copy file from %s to %s: fwrite", src_path, dest_path);
			INIT_ERROR_MSG(tmp);
			return false;
		}
	fclose(src);
	fclose(dest);
	return true; 
}

bool remove_dir(const char *path)
{
	DIR *dir = opendir(path);
	if (dir == NULL)
	{
		INIT_ERROR_MSG("opendir");
		return false;
	}
	string tmp(path);
	if (*tmp.rbegin() != '/')
		tmp.append("/");
	string::size_type tmp_len = tmp.length();
	dirent *pdirent;
	while ((pdirent = readdir(dir)) != NULL)
	{
		if (strcmp(pdirent->d_name, "..") == 0 || strcmp(pdirent->d_name, ".") == 0)
			continue;
		tmp.append(pdirent->d_name);

		struct stat stat_info;
		if (stat(tmp.c_str(), &stat_info))
		{
			closedir(dir);
			INIT_ERROR_MSG("stat");
			return false;
		}

		if (S_ISDIR(stat_info.st_mode))
		{
			if (!remove_dir(tmp.c_str()))
			{
				closedir(dir);
				return false;
			}
		} else
		{
			if (unlink(tmp.c_str()))
			{
				INIT_ERROR_MSG("unlink");
				closedir(dir);
				return false;
			}
		}
		tmp.erase(tmp_len);
	}
	closedir(dir);
	if (rmdir(path))
	{
		INIT_ERROR_MSG("rmdir");
		return false;
	}
	return true;
}


