/*
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef SRC_WRITER_IMPL_SINKIMPL_H_
#define SRC_WRITER_IMPL_SINKIMPL_H_

#include "data/constructs/Key.h"
#include "data/constructs/KeyValue.h"
#include "data/constructs/security/AuthInfo.h"
#include "data/constructs/security/Authorizations.h"
#include "data/extern/concurrentqueue/concurrentqueue.h"
#include "data/constructs/value.h"
#include "scanner/constructs/Results.h"
#include "data/constructs/inputvalidation.h"
#include "data/client/ExtentLocator.h"
#include "data/constructs/client/zookeeperinstance.h"
#include "data/client/LocatorCache.h"
#include "interconnect/ClientInterface.h"
#include "interconnect/tableOps/TableOperations.h"
#include "../Sink.h"
#include "WriterHeuristic.h"
namespace writer
{

/*
 *
 */
class Writer : public Sink<cclient::data::KeyValue>
{
public:
    Writer (cclient::data::Instance *instance,
              interconnect::TableOperations<cclient::data::KeyValue, scanners::ResultBlock<cclient::data::KeyValue>> *tops,
              cclient::data::security::Authorizations *auths, uint16_t threads);
    virtual
    ~Writer ();

    void
    flush (bool override = false);

    void
    setHeuristic (scanners::Heuristic<interconnect::ThriftTransporter> *heuristic)
    {
        writerHeuristic = (WriterHeuristic*) heuristic;
    }
    
    bool enqueue (cclient::data::Mutation *obj)
    {
	   bool enqueued = mutationQueue.enqueue (obj);
	   return enqueued;
    }
    
    bool
    addMutation (std::unique_ptr<cclient::data::Mutation> obj)
    {
	      while(waitingSize() >= ((maxWait()+1)*1.5)) {
		    std::this_thread::sleep_for(std::chrono::milliseconds(25));
	    }
	    
	    cclient::data::Mutation *ptr = obj.release();
	    bool enqueued = enqueue(ptr);
	    if (enqueued && exceedQueue ()) {
		    flush ();
	    }

	    
      return true;
    }
    
    inline virtual size_t
    size ()
    {

	    return currentSize + mutationQueue.size_approx();
    }

protected:
  
  void handleFailures(std::vector<cclient::data::Mutation*> *failures);
  
  
  virtual uint64_t waitingSize()
	{
	  // size of the heuristic is equivalent to the number of workers
	  // currently running.
	  return writerHeuristic->size();
	}
	
	virtual uint64_t maxWait()
	{
	  return writerHeuristic->maxThreads();
	}
	
    WriterHeuristic *writerHeuristic;
    cclient::data::security::AuthInfo *credentials;
    std::vector<interconnect::ClientInterface<interconnect::ThriftTransporter>*> servers;
    cclient::data::zookeeper::ZookeeperInstance *connectorInstance;
    cclient::impl::TabletLocator *tableLocator;
    interconnect::TableOperations<cclient::data::KeyValue, scanners::ResultBlock<cclient::data::KeyValue>> *tops;
    moodycamel::ConcurrentQueue<cclient::data::Mutation*> mutationQueue;
    
};

using BatchWriter = writer::Sink<cclient::data::KeyValue>;

} /* namespace data */

#endif /* SRC_WRITER_IMPL_SINKIMPL_H_ */
