#include "Engine/Core/JobSystem.hpp"

JobSystem* g_theJobSystem = nullptr;

JobWorkerThread::JobWorkerThread(JobSystem* jobSystem, int threadID) :
	m_theJobSystem(jobSystem),
	m_threadID(threadID)
{
	m_thread = new std::thread(&JobWorkerThread::WorkerThreadMain, this);
}

void JobWorkerThread::WorkerThreadMain()
{
	while (!m_isQuitting) {
		Job* pendingJob = m_theJobSystem->ClaimJobToExecute(m_threadJobType);
		if (pendingJob != nullptr) {
			pendingJob->Execute();
			pendingJob->OnFinished();
			m_theJobSystem->MarkJobAsCompleted(pendingJob);
		}
		else {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	}
}

void JobWorkerThread::JoinAndDeleteThread()
{
	m_isQuitting = true;
	m_thread->join();
	delete m_thread;
	m_thread = nullptr;
}



JobSystem::JobSystem(JobSystemConfig const& config) :
	m_config(config)
{
}

JobSystem::~JobSystem()
{

}

void JobSystem::Startup()
{
	m_workerThreads.reserve(m_config.m_amountOfThreads);
	for (int threadId = 0; threadId < m_config.m_amountOfThreads; threadId++) {
		JobWorkerThread* workerThread = new JobWorkerThread(this, threadId);
		m_workerThreads.push_back(workerThread);
	}

	m_jobsOnExecution.reserve(m_config.m_amountOfThreads);
}

void JobSystem::Shutdown()
{
	for (int threadId = 0; threadId < m_config.m_amountOfThreads; threadId++) {
		JobWorkerThread* workerThread = m_workerThreads[threadId];

		if (workerThread) {
			workerThread->JoinAndDeleteThread();
			delete workerThread;
			workerThread = nullptr;
		}
	}
}

void JobSystem::BeginFrame()
{
}

void JobSystem::EndFrame()
{
}

Job* JobSystem::ClaimJobToExecute(int threadJobType)
{
	Job* queuedJob = nullptr;
	m_queuedJobsMutex.lock(); // lock

	if (!m_queuedJobs.empty()) {
		bool foundJob = false;
		std::deque<Job*>::iterator foundJobIt;
		for (std::deque<Job*>::iterator dequeIt = m_queuedJobs.begin(); dequeIt != m_queuedJobs.end() && !foundJob; dequeIt++) {
			if (*dequeIt) {
				Job* job = *dequeIt;
				int jobType = job->m_jobType;
				if ((jobType & threadJobType) != 0) {
					queuedJob = job;
					foundJob = true;
					foundJobIt = dequeIt;
				}
			}
		}
		//queuedJob = m_queuedJobs.front();
		if (foundJob) {
			m_queuedJobs.erase(foundJobIt);
		}
	}

	m_queuedJobsMutex.unlock(); // unlock


	if (queuedJob) {
		m_amountOfQueuedJobs--;
		int executionId = -1;

		m_jobsOnExecutionMutex.lock(); // lock

		for (executionId = 0; executionId < m_jobsOnExecution.size(); executionId++) {
			Job* job = m_jobsOnExecution[executionId];
			if (!job) break;
		}
		if (executionId != -1) {
			if (executionId >= m_jobsOnExecution.size()) {
				m_jobsOnExecution.push_back(queuedJob);
			}
			else {
				m_jobsOnExecution[executionId] = queuedJob;
			}
		}
		else {
			executionId = (int)m_jobsOnExecution.size();
			m_jobsOnExecution.push_back(queuedJob);
		}

		m_jobsOnExecutionMutex.unlock(); // unlock

		queuedJob->m_executionId = executionId;
		m_amountOfExecutingJobs++;

	}

	return queuedJob;
}


void JobSystem::QueueJob(Job* job)
{
	m_queuedJobsMutex.lock();
	m_queuedJobs.push_back(job);
	m_queuedJobsMutex.unlock();

	m_amountOfQueuedJobs++;

}

void JobSystem::MarkJobAsCompleted(Job* job)
{
	if (job->m_executionId < 0) return;
	m_jobsOnExecutionMutex.lock(); // lock

	if (job->m_executionId < m_jobsOnExecution.size()) {
		m_jobsOnExecution[job->m_executionId] = nullptr;
	}

	m_jobsOnExecutionMutex.unlock(); // unlock


	m_completedJobsMutex.lock(); // lock

	m_completedJobs.push_back(job);

	m_completedJobsMutex.unlock(); // unlock

	m_amountOfExecutingJobs--;

}

Job* JobSystem::RetrieveCompletedJob()
{
	m_completedJobsMutex.lock();
	Job* completedJob = nullptr;
	if (!m_completedJobs.empty())
	{
		completedJob = m_completedJobs.front();
		m_completedJobs.pop_front();
	}
	m_completedJobsMutex.unlock();
	return completedJob;
}

void JobSystem::ClearQueuedJobs()
{
	m_amountOfQueuedJobs = 0;
	m_queuedJobsMutex.lock();
	m_queuedJobs.clear();
	m_queuedJobsMutex.unlock();

}

void JobSystem::ClearCompletedJobs()
{
	m_completedJobsMutex.lock();
	m_completedJobs.clear();
	m_completedJobsMutex.unlock();
}

void JobSystem::WaitUntilQueuedJobsCompletion()
{
	bool keepRunning = true;
	while (keepRunning) {
		keepRunning = (m_amountOfExecutingJobs != 0) || (m_amountOfQueuedJobs != 0);

		if (keepRunning) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}
	} // Do nothing while there are queued or running jobs
}

void JobSystem::WaitUntilCurrentJobsCompletion()
{
	bool keepRunning = true;
	while (keepRunning) {
		keepRunning = (m_amountOfExecutingJobs != 0);
		if (keepRunning) {
			std::this_thread::sleep_for(std::chrono::microseconds(1));
		}

	} // Do nothing while there are running jobs
}

void JobSystem::SetThreadJobType(int threadId, int jobType)
{
	if (threadId < 0 || threadId > m_workerThreads.size()) return;
	m_workerThreads[threadId]->m_threadJobType = jobType;
}

Job::Job(int jobType) :
	m_jobType(jobType)
{
}
