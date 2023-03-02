#pragma once
#include <deque>
#include <mutex>
#include <vector>


struct JobSystemConfig {
	int m_amountOfThreads = 0;
};

class Job;
class JobWorkerThread;

constexpr int MULTIPURPOSE_THREAD = ~0;
constexpr int DEFAULT_JOB_ID = MULTIPURPOSE_THREAD;

class JobSystem {

public:

	JobSystem(JobSystemConfig const& config);
	~JobSystem();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	Job* ClaimJobToExecute(int threadJobType);
	void QueueJob(Job* job);
	void MarkJobAsCompleted(Job* job);
	Job* RetrieveCompletedJob();

	void ClearQueuedJobs();
	void ClearCompletedJobs();
	void WaitUntilCurrentJobsCompletion();
	void WaitUntilQueuedJobsCompletion();

	void SetThreadJobType(int threadId, int jobType);

	int GetNumThreads() const { return m_config.m_amountOfThreads; }

private:
	JobSystemConfig m_config;

	std::deque<Job*> m_queuedJobs;
	std::mutex m_queuedJobsMutex;

	std::vector<Job*> m_jobsOnExecution;
	std::mutex m_jobsOnExecutionMutex;

	std::deque<Job*> m_completedJobs;
	std::mutex m_completedJobsMutex;

	std::vector<JobWorkerThread*> m_workerThreads;
	std::atomic<int> m_amountOfExecutingJobs = 0; // Keeps track of current running jobs without having to use mutex + for loop for checking
	std::atomic<int> m_amountOfQueuedJobs = 0; // Keeps track of current running jobs without having to use mutex + for loop for checking

};

class Job {

public:
	Job(int jobType);
	virtual ~Job() {};

	friend class JobWorkerThread;
	friend class JobSystem;
	
public:
	std::atomic<int> m_jobType = -1;

protected:
	virtual void Execute() = 0;
	virtual void OnFinished() = 0;

protected:
	int m_executionId = -1;


};

class JobWorkerThread {

public:
	JobWorkerThread(JobSystem* jobSystem, int threadID);
	~JobWorkerThread() {};

	void WorkerThreadMain();
	void JoinAndDeleteThread();

private:
	friend class JobSystem;
	JobSystem* m_theJobSystem = nullptr;
	std::atomic<bool> m_isQuitting = false;
	int	m_threadID = -1;
	std::thread* m_thread = nullptr;
	int m_threadJobType = MULTIPURPOSE_THREAD; // 0 == Multipurpose as well as all 1s

};