#include "log.h"

#include <atomic>
#include <ctime>
#include <cstring>
#include <fstream>
#include <queue>
#include <tuple>

#include "currentthread_class.h"
#include "noncopyable_class.h"
#include "thread_class.h"
#include "timestamp_class.h"

namespace
{

    /* Returns microseconds since epoch */
   /* uint64_t timestamp_now() //use Timestamp::Now().get_microsecondssinceepoch() instead
    {
    	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }*/

    /* [2016-10-13 00:01:23.528514] */
    void FormatTimestamp(std::ostream & os, uint64_t timestamp)
    {
		std::time_t time_t = timestamp / 1000000;
		auto gmtime = std::gmtime(&time_t);
		char buffer[32];
		strftime(buffer, 32, "%Y-%m-%d %T.", gmtime);
		char microseconds[7];
		sprintf(microseconds, "%06lu", timestamp % 1000000);
		os << '[' << buffer << microseconds << ']';
    }

   /* std::thread::id this_thread_id() //use CurrentThread::Tid() instead, CurrentThread::Tid() equals the value we se use -top in linux cmd
    {
	static thread_local const std::thread::id id = std::this_thread::get_id();
	return id;
    }*/

    template <typename T, typename Tuple>
    struct TupleIndex;

    template <typename T,typename ...Types>
    struct TupleIndex <T, std::tuple<T, Types...>> 
    {
		static constexpr const std::size_t value = 0;
    };

    template <typename T, typename U, typename ...Types>
    struct TupleIndex <T, std::tuple <U, Types...>> 
    {
		static constexpr const std::size_t value = 1 + TupleIndex <T, std::tuple < Types...>>::value;
    };

} // anonymous namespace

namespace tiny_muduo_log
{
    using SupportedTypes = std::tuple <char, uint32_t, uint64_t, int32_t, int64_t, double, LogLine::string_literal_t, char*>;

    char const * ToString(LogLevel loglevel)
    {
		switch (loglevel)
		{
		case LogLevel::INFO:
			return "INFO";
		case LogLevel::WARN:
			return "WARN";
		case LogLevel::CRIT:
			return "CRIT";
		}
		return "XXXX";
    }

    template < typename T >
    void LogLine::EncodeToBufferTail(T t)//don't have to makespace since only called in ctor,we assume that defualt part always lessthan 216 bytes
    {
		*reinterpret_cast<T*>(GetBufferTail()) = t;
		bytesused_ += sizeof(T);
    }

    template < typename T >
    void LogLine::EncodeToBufferTail(T t, uint8_t type)//use in operator<<, the user-write part, may need to makespace
    {
		MakeSpace(sizeof(T) + sizeof(uint8_t));
		EncodeToBufferTail < uint8_t >(type);
		EncodeToBufferTail < T >(t);
    }
	//ctor encode default part to buffer, the user-write part encode through operator<<
    LogLine::LogLine(LogLevel level, char const * file, char const * function, uint32_t line)
	  : bytesused_(0), 
		buffersize_(sizeof(stackbuffer_))
    {
		EncodeToBufferTail < uint64_t >(Timestamp::Now().get_microsecondssinceepoch());
		EncodeToBufferTail < uint32_t >(CurrentThread::Tid());
		EncodeToBufferTail < string_literal_t >(string_literal_t(file));
		EncodeToBufferTail < string_literal_t >(string_literal_t(function));
		EncodeToBufferTail < uint32_t >(line);
		EncodeToBufferTail < LogLevel >(level);
    }

    void LogLine::StringifyFromBufferBegin(std::ostream & os)
    {
		char * begin = nullptr == heapbuffer_ ? stackbuffer_ : heapbuffer_.get();//only one of heapbuffer_ and stackbuffer_ work
		char const * const end = begin + bytesused_;
		//decode default part from buffer
		uint64_t timestamp = *reinterpret_cast < uint64_t * >(begin); begin += sizeof(uint64_t);
		uint32_t threadid = *reinterpret_cast <uint32_t*>(begin); begin += sizeof(uint32_t);
		string_literal_t file = *reinterpret_cast < string_literal_t * >(begin); begin += sizeof(string_literal_t);
		string_literal_t function = *reinterpret_cast < string_literal_t * >(begin); begin += sizeof(string_literal_t);
		uint32_t line = *reinterpret_cast < uint32_t * >(begin); begin += sizeof(uint32_t);
		LogLevel loglevel = *reinterpret_cast < LogLevel * >(begin); begin += sizeof(LogLevel);

		FormatTimestamp(os, timestamp);

		os << '[' << ToString(loglevel) << ']'
		   << '[' << threadid << ']'
		   << '[' << file.s_ << ':' << function.s_ << ':' << line << "] ";
		//decode user-write part
		StringifyRest(os, begin, end);

		os << std::endl;

		if (loglevel >= LogLevel::CRIT)
			os.flush();
    }

    template < typename T >
    char * DecodeFromBuffer(std::ostream & os, char * begin, T * dummy)// pod part
    {
		T t = *reinterpret_cast <T*>(begin);
		os << t;
		return begin + sizeof(T);
    }

    template <>
    char * DecodeFromBuffer(std::ostream & os, char * begin, LogLine::string_literal_t * dummy)// full specialization global function see https://www.jianshu.com/p/add4f01a0de1
    {
		LogLine::string_literal_t s = *reinterpret_cast < LogLine::string_literal_t * >(begin);
		os << s.s_;
		return begin + sizeof(LogLine::string_literal_t);
    }

    template <>
    char * DecodeFromBuffer(std::ostream & os, char * begin, char ** dummy)//use in user-write part, full specialization > partial specialization > origin template
    {
		while (*begin != '\0')
		{
			os << *begin;
			++begin;
		}
		return ++begin;
    }

    void LogLine::StringifyRest(std::ostream & os, char * begin, char const * const end)//decode by type
    {
		if (begin == end)
			return;

		uint8_t type = static_cast <uint8_t>(*begin); begin++;
	
		switch (type)
		{
		case 0:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<0, SupportedTypes>::type*>(nullptr)), end);
			return;
		case 1:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<1, SupportedTypes>::type*>(nullptr)), end);
			return;
		case 2:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<2, SupportedTypes>::type*>(nullptr)), end);
			return;
		case 3:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<3, SupportedTypes>::type*>(nullptr)), end);
			return;
		case 4:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<4, SupportedTypes>::type*>(nullptr)), end);
			return;
		case 5:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<5, SupportedTypes>::type*>(nullptr)), end);
			return;
		case 6:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<6, SupportedTypes>::type*>(nullptr)), end);
			return;
		case 7:
			StringifyRest(os, DecodeFromBuffer(os, begin, static_cast<std::tuple_element<7, SupportedTypes>::type*>(nullptr)), end);
			return;
		}
    }

    char * LogLine::GetBufferTail()
    {
		return heapbuffer_ == nullptr ? &stackbuffer_[bytesused_] : &(heapbuffer_.get())[bytesused_];//only one work
    }
    
    void LogLine::MakeSpace(size_t bytesadditional)
    {
		//operation to bytesused_ done in EncodeToBufferTail()
		size_t const bytesrequired = bytesused_ + bytesadditional;

		if (bytesrequired <= buffersize_)
			return;

		if (nullptr == heapbuffer_)//if bytesrequired > 216, stackbuffer_ will be deprecate,all loads to heapbuffer_
		{
			buffersize_ = std::max(static_cast<size_t>(512), bytesrequired);
			heapbuffer_ = std::make_unique<char[]>(buffersize_);
			memcpy(heapbuffer_.get(), stackbuffer_, bytesused_);
			return;
		}
		else
		{
			buffersize_ = std::max(static_cast<size_t>(1.5 * buffersize_), bytesrequired);
			std::unique_ptr <char[]> newheapbuffer(std::make_unique<char[]>(buffersize_));
			memcpy(newheapbuffer.get(), heapbuffer_.get(), bytesused_);
			heapbuffer_.swap(newheapbuffer);//newheapbuffer will release the old load while out the scope
		}
    }

    void LogLine::EncodeToBufferTail(char const * arg)
    {
		if (arg != nullptr)
			EncodePointer2Const(arg, strlen(arg));
    }

    void LogLine::EncodeToBufferTail(char * arg)
    {
		if (arg != nullptr)
			EncodePointer2Const(arg, strlen(arg));
    }

    void LogLine::EncodePointer2Const(char const * arg, size_t length)
    {
		if (length == 0)
			return;
	
		MakeSpace(1 + length + 1);//type + '\0'
		char * begin = GetBufferTail();
		auto type = TupleIndex <char*, SupportedTypes>::value;
		*reinterpret_cast<uint8_t*>(begin++) = static_cast<uint8_t>(type);
		memcpy(begin, arg, length + 1);
		bytesused_ += 1 + length + 1;
    }

    void LogLine::EncodeToBufferTail(string_literal_t arg)
    {
		EncodeToBufferTail <string_literal_t>(arg, TupleIndex <string_literal_t, SupportedTypes >::value);
    }

    LogLine& LogLine::operator<<(std::string const & arg)
    {
		EncodePointer2Const(arg.c_str(), arg.length());//string.length() == string.size() not include '\0'
		return *this;
    }

    LogLine& LogLine::operator<<(int32_t arg)
    {
		EncodeToBufferTail <int32_t>(arg, TupleIndex<int32_t, SupportedTypes>::value);
		return *this;
    }

    LogLine& LogLine::operator<<(uint32_t arg)
    {
		EncodeToBufferTail <uint32_t>(arg, TupleIndex<uint32_t, SupportedTypes>::value);
		return *this;
    }

    LogLine& LogLine::operator<<(int64_t arg)
    {
		EncodeToBufferTail < int64_t>(arg, TupleIndex <int64_t, SupportedTypes>::value);
		return *this;
    }

    LogLine& LogLine::operator<<(uint64_t arg)
    {
		EncodeToBufferTail <uint64_t>(arg, TupleIndex<uint64_t, SupportedTypes>::value);
		return *this;
    }

    LogLine& LogLine::operator<<(double arg)
    {
		EncodeToBufferTail <double>(arg, TupleIndex<double, SupportedTypes>::value);
		return *this;
    }

    LogLine& LogLine::operator<<(char arg)
    {
		EncodeToBufferTail <char>(arg, TupleIndex<char, SupportedTypes>::value);
		return *this;
    }

    struct BufferBase
    {
		virtual ~BufferBase() = default;
    	virtual void Push(LogLine && logline) = 0;
		virtual bool TryPop(LogLine & logline) = 0;
    };

    struct SpinLock
    {
	public:
		SpinLock(std::atomic_flag & flag) 
		  : flag_(flag)//atomic<bool>
		{
			while (flag_.test_and_set(std::memory_order_acquire));//memory fence see https://github.com/apache/incubator-brpc/blob/master/docs/cn/atomic_instructions.md
		}

		~SpinLock()
		{
			flag_.clear(std::memory_order_release);
		}

	private:
		std::atomic_flag & flag_;
    };

    // Multi Producer Single Consumer RingBuffer :MPSC, SPMC see https://github.com/MengRao/SPMC_Queue
	// used in NonGuaranteedLogger
    class RingBuffer : public BufferBase, public noncopyable
    {
	public:
    	struct alignas(64) SingleLogLine// up-to 64 align,total 256bytes, split to 4 pieces
    	{
		public:
			SingleLogLine() 
			  : flag_{ ATOMIC_FLAG_INIT },
			    written_(0),
			    logline_(LogLevel::INFO, nullptr, nullptr, 0)
			{
			}
	    
			std::atomic_flag flag_;
			char written_;
			char padding_[256 - sizeof(std::atomic_flag) - sizeof(char) - sizeof(LogLine)];
			LogLine logline_;
    	};//class SingleLogLine
	
    	explicit RingBuffer(size_t const size) //how many slot
    	  : size_(size),
    	    ring_(static_cast<SingleLogLine*>(std::malloc(size * sizeof(SingleLogLine)))), // size * 256
            writeindex_(0),
    	    readindex_(0)
    	{
    		for (size_t i = 0; i < size_; ++i)
    		{
    			new (&ring_[i]) SingleLogLine();
    		}
			static_assert(sizeof(SingleLogLine) == 256, "Unexpected size != 256");
    	}

    	~RingBuffer()
    	{
    		for (size_t i = 0; i < size_; ++i)
    		{
    			ring_[i].~SingleLogLine();//why no delete? fixme
    		}
    		std::free(ring_);
    	}

    	void Push(LogLine && logline) override//find 1 slot in buffer ,fill it and set that slot states written, will overwritte written slot
    	{
    		unsigned int writeindex = writeindex_.fetch_add(1, std::memory_order_relaxed) % size_;
    		SingleLogLine & singlelogline = ring_[writeindex];
    		SpinLock spinlock(singlelogline.flag_);
			singlelogline.logline_ = std::move(logline);
			singlelogline.written_ = 1;
    	}

    	bool TryPop(LogLine & logline) override
    	{
    		SingleLogLine & singlelogline = ring_[readindex_ % size_];
    		SpinLock spinlock(singlelogline.flag_);
    		if (singlelogline.written_ == 1)
    		{
    			logline = std::move(singlelogline.logline_);
    			singlelogline.written_ = 0;
				++readindex_;
    			return true;
    		}
    		return false;
    	}

	private:
    	size_t const size_;
    	SingleLogLine * ring_;
    	std::atomic < unsigned int > writeindex_;
		char pad_[64];
    	unsigned int readindex_;//8 + 8 + 4 + 64 + 4 = 88
    };//class ringbuffer


    class GuaranteedLogBuffer : public noncopyable
    {
    public:
    	struct SingleLogLine
    	{
		public:
			SingleLogLine(LogLine && nanologline) 
			  : logline_(std::move(nanologline))
			{}
			char padding_[256 - sizeof(LogLine)];
			LogLine logline_;
    	};// class SingleLogLine
	
    	GuaranteedLogBuffer() 
		  : buffer_(static_cast<SingleLogLine*>(std::malloc(size_ * sizeof(SingleLogLine)))),//32768 * 256 = 8mb
			writtensize_{0}
    	{
    	    for (size_t i = 0; i <= size_; ++i)
    	    {
    			writestate_[i].store(States::EMPTY, std::memory_order_relaxed);
    	    }
			static_assert(sizeof(SingleLogLine) == 256, "Unexpected size != 256");
    	}

    	~GuaranteedLogBuffer()
    	{
			unsigned int writecount = writtensize_.load();
    	    for (size_t i = 0; i < writecount; ++i)
    	    {
    			buffer_[i].~SingleLogLine();//placement new ,we have to dtor manually
    	    }
    	    std::free(buffer_);
    	}

		// Returns true if we need to switch to next buffer
    	bool Push(LogLine && logline, unsigned int const writeindex)
    	{
			//A.new operator : regular new
			//B.operator new : 
			// 1. alloc new memory, no call ctor,throw bad_alloc when failue
			// 2. alloc new memory, no call ctor, return nullptr when failure
			// 3. no new memory, call ctor in point memory 
			//C.placement new
			new (&buffer_[writeindex]) SingleLogLine(std::move(logline));//problemmark operator new ,manually ctor in givem memory buffer_[writeindex]
			writestate_[writeindex].store(States::FILLED, std::memory_order_release);
			return writtensize_.fetch_add(1, std::memory_order_acquire) + 1 == size_;//return value before fetch_add(1),so we have to add 1 again
    	}

    	bool TryPop(LogLine & logline, unsigned int const readindex)
    	{
			if (States::FILLED == writestate_[readindex].load(std::memory_order_acquire))
			{
				SingleLogLine & singlelogline = buffer_[readindex];
				logline = std::move(singlelogline.logline_);
				return true;
			}
			return false;
    	}

		static constexpr const size_t size_ = 32768; // numbers of slot, totally 8MB. Helps reduce memory fragmentation

    private:
		enum class States : bool {EMPTY = false ,FILLED = true};
    	SingleLogLine * buffer_;
		std::atomic<States> writestate_[size_];
		std::atomic<uint32_t> writtensize_;
    };//class GuaranteedLogBuffer

    class QueueBuffer : public BufferBase , public noncopyable
    {
    public:
		QueueBuffer()
		  : currentreadbuffer_{nullptr},
		    writeindex_(0),
		    flag_{ATOMIC_FLAG_INIT},
		    readindex_(0)
		{
			SetupNextWriteBuffer();
		}

    	void Push(LogLine && logline) override
    	{
    	    unsigned int writeindex = writeindex_.fetch_add(1, std::memory_order_relaxed);
			//multi thread write sametime may cause consumer slower than producer, so writeindex may accumulate and bigger than GuaranteedLogBuffer::size_
			//so we need a 'else' below, wait for consumer to set writeindex_ to zero in SetupNextWriteBuffer() 
			if (writeindex < GuaranteedLogBuffer::size_)//current GuaranteedLogBuffer not full
			{
				if (currentwritebuffer_.load(std::memory_order_acquire)->Push(std::move(logline), writeindex))//full
				{
					SetupNextWriteBuffer();
				}
			}
			else//loop again
			{
				while (writeindex_.load(std::memory_order_acquire) >= GuaranteedLogBuffer::size_);
					Push(std::move(logline));
			}
    	}

    	bool TryPop(LogLine & logline) override
		{
			if (currentreadbuffer_ == nullptr)
				currentreadbuffer_ = GetNextReadBuffer();

			GuaranteedLogBuffer * readbuffer = currentreadbuffer_;

			if (readbuffer == nullptr)
				return false;

			if (bool success = readbuffer->TryPop(logline, readindex_))
			{
				readindex_++;
				if (readindex_ == GuaranteedLogBuffer::size_)
				{
					readindex_ = 0;
					currentreadbuffer_ = nullptr;
					SpinLock spinlock(flag_);
					buffer_.pop();
				}
				return true;
			}

			return false;
		}

    private:
		void SetupNextWriteBuffer()
		{
			std::unique_ptr<GuaranteedLogBuffer> nextwritebuffer(std::make_unique<GuaranteedLogBuffer>());
			currentwritebuffer_.store(nextwritebuffer.get(), std::memory_order_release);
			SpinLock spinlock(flag_);
			buffer_.push(std::move(nextwritebuffer));
			writeindex_.store(0, std::memory_order_relaxed);
		}
	
		GuaranteedLogBuffer * GetNextReadBuffer()
		{
			SpinLock spinlock(flag_);
			return buffer_.empty() ? nullptr : buffer_.front().get();
		}

    private:
		std::queue<std::unique_ptr<GuaranteedLogBuffer>> buffer_;//8MB each
    	std::atomic <GuaranteedLogBuffer*> currentwritebuffer_;
		GuaranteedLogBuffer* currentreadbuffer_;
    	std::atomic<unsigned int> writeindex_;//readindex_ and writeindex_ related to a single GuaranteedLogBufferin in queue
		std::atomic_flag flag_;
    	unsigned int readindex_;
    };//class QueueBuffer

    class FileWriter
    {
	public:
		FileWriter(std::string const & logdirectory, std::string const & logfilename, uint32_t logfilerollsizemb)
		  : logfilerollsizebytes_(logfilerollsizemb * 1024 * 1024),
			name_(logdirectory + logfilename)
		{
			RollFile();
		}
	
	void Write(LogLine & logline)
	{
	    auto pos = os_->tellp();//current write index
	    logline.StringifyFromBufferBegin(*os_);
	    writtenbytes += os_->tellp() - pos;
	    if (writtenbytes > logfilerollsizebytes_)
	    {
			RollFile();
	    }
	}

    private:
		void RollFile()
		{
			if (os_)
			{
				os_->flush();
				os_->close();
			}

			writtenbytes = 0;
			//reset : will delete  source holding now, then manage the new one(if new one is not nullptr)
			//release : return holding source and give up control
			//get : return holding source and  NOT give up control
			os_.reset(std::make_unique<std::ofstream>().release());
			// TODO Optimize this part. Does it even matter ?
			std::string logfilename = name_;
			logfilename.append(".");
			logfilename.append(std::to_string(++filenumber_));
			logfilename.append(".txt");
			os_->open(logfilename, std::ofstream::out | std::ofstream::trunc/*rewrite exist part*/);
		}

    private:
		uint32_t filenumber_ = 0;
		std::streamoff writtenbytes = 0;
		uint32_t const logfilerollsizebytes_;
		std::string const name_;
		std::unique_ptr <std::ofstream> os_;
    };//class FileWriter

    class NanoLogger
    {
    public:
		NanoLogger(NonGuaranteedLogger ngl, std::string const & logdirectory, std::string const & logfilename, uint32_t logfilerollsizemb)
		  : state_(State::INIT),
			bufferbase_(std::make_unique<RingBuffer>(std::max(1u, ngl.ringbuffersizemb_) * 1024 * 4)),//ringbuffersizemb_ * 1024 * 1024 / 256
			filewriter_(logdirectory, logfilename, std::max(1u, logfilerollsizemb)),
			thread_(std::bind(&NanoLogger::Pop, this),"logthread")
		{
			thread_.Start();
			state_.store(State::READY, std::memory_order_release);
		}

		NanoLogger(GuaranteedLogger gl, std::string const & logdirectory, std::string const & logfilename, uint32_t logfilerollsizemb)
	      : state_(State::INIT),
			bufferbase_(std::make_unique<QueueBuffer>()),
			filewriter_(logdirectory, logfilename, std::max(1u, logfilerollsizemb)),
			thread_(std::bind(&NanoLogger::Pop, this), "logthread")
		{
			thread_.Start();
			state_.store(State::READY, std::memory_order_release);//ensure code before this line won't be re order to after
		}

		~NanoLogger()
		{
			state_.store(State::SHUTDOWN);
			thread_.Join();//wait for thread:pop to finish, means write all remaining log to file
		}

		void Add(LogLine && logline)
		{
			bufferbase_->Push(std::move(logline));
		}
	
		void Pop()
		{
			// Wait for constructor to complete and pull all stores done there to this thread / core.
			while (state_.load(std::memory_order_acquire) == State::INIT)
				CurrentThread::SleepMicroSeconds(50);
	    
			LogLine logline(LogLevel::INFO, nullptr, nullptr, 0);

			while(state_.load() == State::READY)//ctor done, running
			{
				if (bufferbase_->TryPop(logline))
					filewriter_.Write(logline);
				else
					//std::this_thread::sleep_for(std::chrono::microseconds(50));
				    CurrentThread::SleepMicroSeconds(50);
			}
	    
			// pop and log all remaining entries, on dtor
			while(bufferbase_->TryPop(logline))
			{
				filewriter_.Write(logline);
			}
		}
	
    private:
		enum class State : uint8_t
		{
			INIT,
			READY,
			SHUTDOWN
		};

		std::atomic<State> state_;
		std::unique_ptr<BufferBase> bufferbase_;
		FileWriter filewriter_;
		Thread thread_;
    };//class NanoLogger

	// Initialize logger
    std::unique_ptr < NanoLogger > nanologger;
    std::atomic < NanoLogger * > atomicnanologger;

	//add to buffer(ringbuffer/queuebuffer) through operator==
    bool NanoLog::operator==(LogLine & logline)
    {
		atomicnanologger.load(std::memory_order_acquire)->Add(std::move(logline));
		return true;
    }

    void Initialize(NonGuaranteedLogger ngl, std::string const & logdirectory, std::string const & logfilename, uint32_t logfilerollsizemb)
    {
		nanologger.reset(std::make_unique<NanoLogger>(ngl, logdirectory, logfilename, logfilerollsizemb).release());
		atomicnanologger.store(nanologger.get(), std::memory_order_seq_cst);
    }

    void Initialize(GuaranteedLogger gl, std::string const & logdirectory, std::string const & logfilename, uint32_t logfilerollsizemb)
    {
		nanologger.reset(std::make_unique <NanoLogger>(gl, logdirectory, logfilename, logfilerollsizemb).release());
		atomicnanologger.store(nanologger.get(), std::memory_order_seq_cst);
    }

	//change loglevel in runtime
    std::atomic <unsigned int> loglevel = {0};

    void SetLogLevel(LogLevel level)
    {
		loglevel.store(static_cast<unsigned int>(level), std::memory_order_release);
    }       

    bool ShallLog(LogLevel level)
    {
		return static_cast<unsigned int>(level) >= loglevel.load(std::memory_order_relaxed);
    }

} // namespace nanologger
