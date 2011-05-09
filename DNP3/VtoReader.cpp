/*
 * Licensed to Green Energy Corp (www.greenenergycorp.com) under one or more
 * contributor license agreements. See the NOTICE file distributed with this
 * work for additional information regarding copyright ownership.  Green Enery
 * Corp licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include "VtoReader.h"

#include <APL/Exception.h>
#include <APL/Logger.h>

#include <boost/foreach.hpp>

namespace apl {
	namespace dnp {

		void VtoReader::AddVtoChannel(IVtoCallbacks* apCallbacks)
		{
			/*
			 * The whole function is thread-safe, from start to finish.
			 */
			CriticalSection cs(&mLock);

			boost::uint8_t id = apCallbacks->GetChannelId();

			/* Has this channel id already been registered? */
			if (mChannelMap.find(id) != mChannelMap.end())
			{
				std::stringstream out;
				out << (int)id;

				throw ArgumentException(
				     LOCATION,
				    "Channel already registered: " +
				    out.str() );
			}

			/* Register the callbacks for the channel id */
			mChannelMap[id] = apCallbacks;
		}

		void VtoReader::RemoveVtoChannel(IVtoCallbacks* apCallbacks)
		{
			/*
			 * The whole function is thread-safe, from start to finish.
			 */
			CriticalSection cs(&mLock);

			boost::uint8_t id = apCallbacks->GetChannelId();

			if (mChannelMap.erase(id) == 0)
			{
				std::stringstream out;
				out << (int)id;

				throw ArgumentException( LOCATION,
				                        "Channel not registered: " + out.str() );
			}
		}

		void VtoReader::Update(const VtoData& arData,
		                       boost::uint8_t aChannelId)
		{
			/* Make sure we are part of the larger DNP3 transaction */
			assert( this->InProgress() );

			/*
			 * Lookup the callback object for the channel id.  If it doesn't
			 * exist, register an error.  Otherwise, notify the callback
			 * object.
			 */

			ChannelMap::iterator i = mChannelMap.find(aChannelId);

			if (i == mChannelMap.end())
			{
				LOG_BLOCK(LEV_ERROR,
				          "No registered callback handler for received data "
				          "on VTO channel id: " + aChannelId);
			}
			else
			{
				i->second->OnDataReceived(arData.GetData(), arData.GetSize());
			}
		}

		void VtoReader::_Start()
		{
			mLock.Lock();
		}

		void VtoReader::_End()
		{
			mLock.Unlock();
		}

		void VtoReader::Notify(size_t aAvailableBytes)
		{
			/*
			 * The whole function is thread-safe, from start to finish.
			 */
			CriticalSection cs(&mLock);

			for (ChannelMap::iterator i = mChannelMap.begin();
			     i != mChannelMap.end();
			     ++i)
			{
				i->second->OnBufferAvailable(aAvailableBytes);
			}
		}
	}
}

/* vim: set ts=4 sw=4: */

