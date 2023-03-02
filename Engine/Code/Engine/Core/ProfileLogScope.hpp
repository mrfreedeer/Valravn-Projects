#pragma once
#include <stdint.h>

class ProfileLogScope {
public:
	ProfileLogScope(char const* tag, bool logToConsole = true, uint64_t* reportTo = nullptr);

	~ProfileLogScope();

private:
	char const* m_tag;
	uint64_t* m_reportTo = nullptr;
	uint64_t m_start;
	bool m_logToConsole = true;
};

#define PROFILE_LOG_SCOPE(tag) ProfileLogScope __log_scope_##tag(#tag);