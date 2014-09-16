
#include "../../yuni.h"
#include "waitingroom.h"


namespace Yuni
{
namespace Private
{
namespace QueueService
{


	WaitingRoom::~WaitingRoom()
	{
		// locking all mutex to prevent some race conditions
		// (with clear() for example)
		for (uint i = 0; i != (uint) priorityCount; ++i)
			pMutexes[i].lock();
		for (uint i = 0; i != (uint) priorityCount; ++i)
			pMutexes[i].unlock();
	}


	void WaitingRoom::clear()
	{
		if (YUNI_LIKELY(pJobCount != 0))
		{
			// we should lock all lists before anything
			for (uint i = 0; i != (uint) priorityCount; ++i)
			{
				pMutexes[i].lock();
				hasJob[i] = false;
			}

			// reset the total number of job _before_ unlocking
			pJobCount = 0; // may notify listeners that there is nothing to do

			// clear
			for (uint i = 0; i != (uint) priorityCount; ++i)
				pJobs[i].clear();

			// unlock all
			for (uint i = 0; i != (uint) priorityCount; ++i)
				pMutexes[i].unlock();
		}
	}


	void WaitingRoom::add(const Yuni::Job::IJob::Ptr& job)
	{
		// Locking the priority queue
		// We should avoid ThreadingPolicy::MutexLocker since it may not be
		// the good threading policy for these mutexes
		Yuni::MutexLocker locker(pMutexes[priorityDefault]);

		// Resetting some internal variables of the job
		Yuni::Private::QueueService::JobAccessor<Yuni::Job::IJob>::AddedInTheWaitingRoom(*job);
		// Adding it into the good priority queue
		pJobs[priorityDefault].push_back(job);

		// Resetting our internal state
		hasJob[priorityDefault] = true;
		++pJobCount;
	}


	void WaitingRoom::add(const Yuni::Job::IJob::Ptr& job, Yuni::Job::Priority priority)
	{
		// Locking the priority queue
		// We should avoid ThreadingPolicy::MutexLocker since it may not be
		// the good threading policy for these mutexes
		Yuni::MutexLocker locker(pMutexes[priority]);

		// Resetting some internal variables of the job
		Yuni::Private::QueueService::JobAccessor<Yuni::Job::IJob>::AddedInTheWaitingRoom(*job);
		// Adding it into the good priority queue
		pJobs[priority].push_back(job);

		// Resetting our internal state
		hasJob[priority] = true;
		++pJobCount;
	}


	bool WaitingRoom::pop(Yuni::Job::IJob::Ptr& out, const Yuni::Job::Priority priority)
	{
		// We should avoid ThreadingPolicy::MutexLocker since it may not be
		// the good threading policy for these mutexes
		Yuni::MutexLocker locker(pMutexes[priority]);

		if (not pJobs[priority].empty())
		{
			// It remains at least one job to run !
			out = pJobs[priority].front();
			// Removing it from the list of waiting jobs
			pJobs[priority].pop_front();
			// Resetting atomic variables about the internal status
			hasJob[priority] = (not pJobs[priority].empty());

			--pJobCount;
			return true;
		}

		// It does not remain any job for this priority. Aborting.
		// Resetting some variable
		hasJob[priority] = false;
		return false;
	}





} // namespace QueueService
} // namespace Private
} // namespace Yuni

