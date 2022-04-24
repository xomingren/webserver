#pragma once

#include <concepts>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <type_traits>
// Ensure initialize is called once prior to logging.
 // This will create log files like /tmp/log1.txt, /tmp/log2.txt etc.
 // Log will roll to the next file after every 1MB.
 // This will initialize the guaranteed logger.
//tiny_muduo_log::Initialize(nanolog::GuaranteedLogger(), "/tmp/", "log", 1);

// Or if you want to use the non guaranteed logger -
// ringbuffersizemb - LogLines are pushed into a mpsc ring buffer whose size
// is determined by this parameter. Since each LogLine is 256 bytes,
// ringbuffersize = ringbuffersizemb * 1024 * 1024 / 256
// In this example ringbuffersizemb = 3.
// tiny_muduo_log::Initialize(nanolog::NonGuaranteedLogger(3), "/tmp/", "log", 1);
namespace tiny_muduo_log
{

    enum class LogLevel : uint8_t { INFO, WARN, CRIT };
    
	template<typename T>
	concept pointer2constchar = std::is_same < T, char const* >::value;

	template<typename T>
	concept pointer2char = std::is_same < T, char* >::value;

    class LogLine
    {
	public:
		LogLine(LogLevel level, char const * file, char const * function, uint32_t line);

		LogLine(LogLine &&) = default;
		LogLine& operator=(LogLine &&) = default;

		//buffer decode to os
		void StringifyFromBufferBegin(std::ostream & os);

		//put those types into buffer
		LogLine& operator<<(char arg);
		LogLine& operator<<(int32_t arg);
		LogLine& operator<<(uint32_t arg);
		LogLine& operator<<(int64_t arg);
		LogLine& operator<<(uint64_t arg);
		LogLine& operator<<(double arg);
		LogLine& operator<<(std::string const & arg);

		template < size_t N >
		LogLine& operator<<(const char (&arg)[N])
		{
			EncodeToBufferTail(string_literal_t(arg));
			return *this;
		}

		//c++ 11 version
		// 1. enabled via the return type
		//template < typename Arg >
		//typename std::enable_if < std::is_same < Arg, char const * >::value, LogLine& >::type//if(arg == char const) typename type = &LogLine;
		//operator<<(Arg const & arg)
		//{
		//    EncodeToBufferTail(arg);
		//    return *this;
		//}
		//2. enabled via a parameter
		//3. enabled via a template parameter
		//template < typename Arg >
		//typename std::enable_if < std::is_same < Arg, char * >::value, LogLine& >::type
		//operator<<(Arg const & arg)
		//{
		//    EncodeToBufferTail(arg);
		//    return *this;
		//}
	
		//c++ 20 version
	
		template <pointer2constchar T>
		LogLine& operator<<(T const & arg)
		{
			EncodeToBufferTail(arg);
			return *this;
		}

		template <pointer2char T>
		LogLine& operator<<(T const& arg)
		{
			EncodeToBufferTail(arg);
			return *this;
		}

		struct string_literal_t
		{
			explicit string_literal_t(char const * s) : s_(s) {}
			char const * s_;
		};

	private:	
		char * GetBufferTail();

		//append to buffer tail
		template < typename T >
		void EncodeToBufferTail(T arg);

		template < typename T >
		void EncodeToBufferTail(T arg, uint8_t type_id);

		void EncodeToBufferTail(char * arg);
		void EncodeToBufferTail(char const * arg);//what's thr difference between EncodeToBufferTail(string_literal_t arg)?
		void EncodeToBufferTail(string_literal_t arg);//encode pointer
		void EncodePointer2Const(char const * arg, size_t length);//encode value not poniter

		void MakeSpace(size_t additional_bytes);
		void StringifyRest(std::ostream & os, char * start, char const * const end);

		private:
		size_t bytesused_;
		size_t buffersize_;//size avaliable before makespace, default 216 in linux64
		std::unique_ptr <char[]> heapbuffer_;
		char stackbuffer_[256 - 2 * sizeof(size_t) - sizeof(decltype(heapbuffer_)) - 8 /* reserved */];
		//char* p = new int(10) <==> unique_ptr<int>p(make_unique<int>(10))
		//char* arr = new int[10] <==> unique_ptr<int>p(make_unique<int[]>(10))
    }; //class LogLine
    
    struct NanoLog
    {
	/*
	 * Ideally this should have been operator+=
	 * Could not get that to compile, so here we are...
	 */
		bool operator==(LogLine &);//add to buffer through operator==
    };//class NanoLog

   


    /*
     * Non guaranteed logging. Uses a ring buffer to hold log lines.
     * When the ring gets full, the previous LogLine in the slot will be dropped.
     * Does not block producer even if the ring buffer is full.
     * ringbuffersizemb - LogLines are pushed into a mpsc ring buffer whose size
     * is determined by this parameter. Since each LogLine is 256 bytes, 
     * ringbuffersize(how many slot) = ringbuffersizemb * 1024 * 1024 / 256
     */
    struct NonGuaranteedLogger
    {
		explicit NonGuaranteedLogger(uint32_t ringbuffersizemb_) 
	      : ringbuffersizemb_(ringbuffersizemb_)
		{}
		uint32_t ringbuffersizemb_;
    };//class NonGuaranteedLogger

    /*
     * Provides a guarantee log lines will not be dropped. 
     */
    struct GuaranteedLogger
    {
    };//class GuaranteedLogger
    
    /*initial 
     * Ensure Initialize() is called prior to any log statements.
     * logdirectory - where to create the logs. For example - "/tmp/"
     * logfilename - root of the file name. For example - "tinymuduolog"
     * This will create log files of the form -
     * /tmp/tinymuduolog.1.txt
     * /tmp/tinymuduolog.2.txt
     * etc.
     * logfilerollsizemb - mega bytes after which we roll to next log file.
     */
    void Initialize(GuaranteedLogger gl, std::string const & logdirectory, std::string const & logfilename, uint32_t logfilerollsizemb);
    void Initialize(NonGuaranteedLogger ngl, std::string const & logdirectory, std::string const & logfilename, uint32_t logfilerollsizemb);

	//change loglevel in runtime
	void SetLogLevel(LogLevel level);

	bool ShallLog(LogLevel level);

} // namespace tiny_muduo_log

#define LOG_INFO tiny_muduo_log::ShallLog(tiny_muduo_log::LogLevel::INFO) && tiny_muduo_log::NanoLog() == tiny_muduo_log::LogLine(tiny_muduo_log::LogLevel::INFO, __FILE__, __func__, __LINE__)
#define LOG_WARN tiny_muduo_log::ShallLog(tiny_muduo_log::LogLevel::WARN) && tiny_muduo_log::NanoLog() == tiny_muduo_log::LogLine(tiny_muduo_log::LogLevel::WARN, __FILE__, __func__, __LINE__)
#define LOG_CRIT tiny_muduo_log::ShallLog(tiny_muduo_log::LogLevel::CRIT) && tiny_muduo_log::NanoLog() == tiny_muduo_log::LogLine(tiny_muduo_log::LogLevel::CRIT, __FILE__, __func__, __LINE__)


