/* Copyright 2013-2018 MultiMC Contributors
 *
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

#pragma once

#include <launch/LaunchStep.h>
#include <minecraft/auth/MojangAccount.h>

class ClaimAccount: public LaunchStep
{
	Q_OBJECT
public:
	explicit ClaimAccount(LaunchTask *parent, AuthSessionPtr session);
	virtual ~ClaimAccount() {};

	void executeTask() override;
	void finalize() override;
	bool canAbort() const override
	{
		return false;
	}
private:
	std::unique_ptr<UseLock> lock;
	MojangAccountPtr m_account;
};
