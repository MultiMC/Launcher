#pragma once

#include <memory>
#include <functional>

#include <QObject>
#include <QMetaProperty>

class QVariant;
class Resource;

/// Base class for things that can use a resource
class ResourceObserver
{
public:
	virtual ~ResourceObserver();

protected: // these methods are called by the Resource when something changes
	virtual void resourceUpdated() = 0;
	virtual void setFailure(const QString &) {}
	virtual void setProgress(const int) {}

private:
	friend class Resource;
	void setSource(std::shared_ptr<Resource> resource) { m_resource = resource; }

protected:
	template<typename T>
	T get() const { return getInternal(qMetaTypeId<T>()).template value<T>(); }
	QVariant getInternal(const int typeId) const;

private:
	std::shared_ptr<Resource> m_resource;
};

/** Observer for QObject properties
 *
 * Give it a target and the name of a property, and that property will be set when the resource changes.
 *
 * If no name is given an attempt to find a default property for some common classes is done.
 */
class QObjectResourceObserver : public QObject, public ResourceObserver
{
public:
	explicit QObjectResourceObserver(QObject *target, const char *property = nullptr);

	void resourceUpdated() override;

private:
	QObject *m_target;
	QMetaProperty m_property;
};

template <typename Ret, typename Arg, typename Func>
class FunctionResourceObserver : public ResourceObserver
{
	std::function<Ret(Arg)> m_function;
public:
	template <typename T>
	explicit FunctionResourceObserver(T &&func)
		: m_function(std::forward<Func>(func)) {}

	void resourceUpdated() override
	{
		m_function(get<Arg>());
	}
};
