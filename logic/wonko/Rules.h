/* Copyright 2013-2015 MultiMC Contributors
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

#include <QString>
#include <QList>
#include <memory>
#include "OpSys.h"

class QJsonValue;
using RulesPtr = std::shared_ptr<class Rules>;

struct RuleContext
{
	QString side;
};

class BaseRule
{
public:
	enum RuleAction
	{
		Allow,
		Disallow,
		Defer
	};

	BaseRule(const RuleAction result) : m_result(result)
	{
	}
	virtual ~BaseRule() {}

	virtual bool applies(const RuleContext &ctxt) const = 0;

	QString resultToString() const;
	RuleAction result() const { return m_result; }

	static RuleAction actionFromString(const QString &str);

private:
	RuleAction m_result;
};

class OsRule : public BaseRule
{
public:
	explicit OsRule(const RuleAction result, const OpSys system, const QString &version_regexp, const int arch = -1)
		: BaseRule(result), m_system(system), m_version_regexp(version_regexp), m_arch(arch)
	{
	}

	bool applies(const RuleContext &) const override
	{
		return m_system == OpSys::currentSystem();
	}

	// the OS
	OpSys m_system;
	// the OS version regexp
	QString m_version_regexp;
	// architecture (32 or 64)
	int m_arch;
};

class ImplicitRule : public BaseRule
{
public:
	explicit ImplicitRule(const RuleAction result) : BaseRule(result)
	{
	}
	bool applies(const RuleContext &) const override
	{
		return true;
	}
};
class SidedRule : public BaseRule
{
public:
	explicit SidedRule(const RuleAction result, const QString &side)
		: BaseRule(result), m_side(side)
	{
	}
	bool applies(const RuleContext &ctxt) const override
	{
		return !ctxt.side.isNull() && m_side == ctxt.side;
	}

private:
	QString m_side;
};

class Rules
{
public:
	explicit Rules(const QList<std::shared_ptr<BaseRule>> &rules);
	explicit Rules();

	void load(const QJsonValue &val);

	/// merge the rules from other into this
	void merge(RulesPtr &other);

	BaseRule::RuleAction result(const RuleContext &ctxt = RuleContext()) const;

	QList<std::shared_ptr<BaseRule>> rules() const
	{
		return m_rules;
	}

private:
	QList<std::shared_ptr<BaseRule>> m_rules;
};
