#pragma once

namespace logger
{

void debug(const char *fmt, ...);
void info(const char *fmt, ...);
void warning(const char *fmt, ...);
void error(const char *fmt, ...);

} // namespace logger
