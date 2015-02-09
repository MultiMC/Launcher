#pragma once

#include <QString>
#include <string>
#include <fstream>

#include <thread>
#include <mutex>

class Log
{
	public:
		enum Type
		{
			Info,
			Warn,
			Error
		};

		Log();
		~Log();

		void open(const std::string& path);

		/** Write @p text to the log.  This method is thread-safe. */
		void write(Type type, const std::string& text);
		/** Write @p text to the log.  This method is thread-safe. */
		void write(Type type, const char* text);
		/** Write @p text to the log.  This method is thread-safe. */
		void write(Type type, const QString &text);

		static Log* instance();
	
	private:
		static void writeToStream(std::ostream& stream, Type type, const char* text);

		std::mutex m_mutex;
		std::ofstream m_output;
};

inline void Log::write(Type type, const std::string& text)
{
	write(type,text.c_str());
}

inline void Log::write(Log::Type type, const QString &text)
{
	write(type, text.toLocal8Bit().constData());
}

#define LOG(type,text) \
  Log::instance()->write(Log::type,text)


