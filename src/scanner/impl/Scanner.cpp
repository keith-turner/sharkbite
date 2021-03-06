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

#include <algorithm>
#include <map>
#include <string>
#include <set>



#include "../../../include/scanner/impl/../Source.h"
#include "../../../include/scanner/impl/../../data/constructs/Key.h"
#include "../../../include/scanner/impl/../../data/constructs/security/AuthInfo.h"
#include "../../../include/scanner/impl/../../data/constructs/security/Authorizations.h"
#include "../../../include/scanner/impl/../../data/constructs/value.h"
#include "../../../include/scanner/impl/../constructs/Results.h"
#include "../../../include/scanner/impl/../../data/constructs/inputvalidation.h"
#include "../../../include/scanner/impl/../../data/client/ExtentLocator.h"
#include "../../../include/scanner/impl/../../data/constructs/client/zookeeperinstance.h"
#include "../../../include/scanner/impl/../../data/client/LocatorCache.h"
#include "../../../include/scanner/impl/../constructs/ServerHeuristic.h"
#include "../../../include/scanner/impl/../../interconnect/ClientInterface.h"
#include "../../../include/scanner/impl/../../interconnect/tableOps/TableOperations.h"
#include "../../../include/scanner/impl/Scanner.h"

namespace scanners
{

Scanner::Scanner (cclient::data::Instance *instance,
                  interconnect::TableOperations<cclient::data::KeyValue, ResultBlock<cclient::data::KeyValue>> *tops,
                  cclient::data::security::Authorizations *auths, uint16_t threads) :
    scannerAuths (auths)
{
    
    connectorInstance = dynamic_cast<cclient::data::zookeeper::ZookeeperInstance*> (instance);
    resultSet = NULL;
    tableLocator = cclient::impl::cachedLocators.getLocator (
                       cclient::impl::LocatorKey (connectorInstance, tops->getTableId ()));
    scannerHeuristic = new ScannerHeuristic (threads);
    credentials = tops->getCredentials ();
}

}
