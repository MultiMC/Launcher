#include "Rules.h"

#include "Json.h"

Rules::Rules(const QList<std::shared_ptr<BaseRule> > &rules)
	: m_rules(rules)
{
}
Rules::Rules()
{
}

void Rules::load(const QJsonValue &val)
{
	using namespace Json;

	QList<std::shared_ptr<BaseRule>> result;

	for (const QJsonObject &ruleObj : ensureIsArrayOf<QJsonObject>(val))
	{
		const BaseRule::RuleAction action = BaseRule::actionFromString(ensureString(ruleObj, "action"));
		if (action == BaseRule::Defer)
		{
			throw JsonException("Unable to parse 'action' for a rule");
		}

		if (ruleObj.contains("os"))
		{
			const QJsonObject osObj = ensureObject(ruleObj, "os");
			result.append(std::make_shared<OsRule>(action,
												   OpSys::fromString(ensureString(osObj, "name")),
												   ensureString(osObj, "version", ""),
												   ensureString(osObj, "arch", "-1").toInt()));
		}
		else if (ruleObj.contains("side"))
		{
			result.append(std::make_shared<SidedRule>(action, ensureString(ruleObj, "side")));
		}
		else
		{
			result.append(std::make_shared<ImplicitRule>(action));
		}
	}

	m_rules.swap(result);
}

void Rules::merge(RulesPtr &other)
{
	for (std::shared_ptr<BaseRule> rule : other->m_rules)
	{
		if (!std::dynamic_pointer_cast<ImplicitRule>(rule))
		{
			m_rules.append(rule);
		}
	}
}

BaseRule::RuleAction Rules::result(const RuleContext &ctxt) const
{
	BaseRule::RuleAction out = BaseRule::Defer;
	for (std::shared_ptr<BaseRule> rule : m_rules)
	{
		if (rule->applies(ctxt) && rule->result() != BaseRule::Defer)
		{
			out = rule->result();
		}
	}
	return out;
}


QString BaseRule::resultToString() const
{
	switch (m_result)
	{
	case Allow: return "allow";
	case Disallow: return "disallow";
	default: return QString();
	}
}

BaseRule::RuleAction BaseRule::actionFromString(const QString &str)
{
	if (str == "allow")
	{
		return Allow;
	}
	else if (str == "disallow")
	{
		return Disallow;
	}
	else
	{
		return Defer;
	}
}
