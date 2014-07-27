/* Copyright 2014 Jan Dalheimer
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

#include <QObject>
#include <QMetaMethod>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>
#include <memory>

class Bindable : public QObject
{
	Q_OBJECT
public:
	Bindable(Bindable *parent = 0) : QObject(parent)
	{
		// inherit the bindings from parent
		bind(parent);
	}
	Bindable(QObject *parent = 0) : QObject(parent)
	{
	}
	virtual ~Bindable()
	{
	}

	void bind(Bindable *task)
	{
		for (auto it = task->m_bindings.constBegin(); it != task->m_bindings.constEnd(); ++it)
		{
			if (m_bindings.contains(it.key()))
			{
				continue;
			}
			m_bindings.insert(it.key(), Binding(it.value().receiver, it.value().method, task));
		}
	}
	void unbind(Bindable *task)
	{
		for (const auto key : task->m_bindings.keys())
		{
			if (m_bindings.contains(key) && m_bindings[key].source == task)
			{
				m_bindings.remove(key);
			}
		}
	}

	void bind(const QString &id, QObject *receiver, const char *methodSignature)
	{

		auto mo = receiver->metaObject();
		const QMetaMethod method = mo->method(mo->indexOfMethod(
			QMetaObject::normalizedSignature(methodSignature + 1).constData()));
		Q_ASSERT_X(method.isValid(), "Bindable::bind", "Invalid method signature");
		m_bindings.insert(id, Binding(receiver, method));
	}
	void unbind(const QString &id)
	{
		m_bindings.remove(id);
	}

private:
	struct Binding
	{
		Binding(QObject *receiver, const QMetaMethod &method, Bindable *source = 0)
			: receiver(receiver), method(method), source(source)
		{
		}
		Binding()
		{
		}
		QObject *receiver;
		QMetaMethod method;
		Bindable *source = 0;
	};
	QMap<QString, Binding> m_bindings;

protected:
	template <typename Ret, typename... Params> Ret wait(const QString &id, Params... params)
	{
		Q_ASSERT(m_bindings.contains(id));
		QVariantList({qMetaTypeId<Params>()...});
		const auto binding = m_bindings[id];
		const QMetaMethod method = binding.method;
		Q_ASSERT_X(method.parameterCount() == sizeof...(params), "Bindable::wait",
				   qPrintable(QString("Incompatible argument count (expected %1, got %2)")
								  .arg(method.parameterCount(), sizeof...(params))));
		Q_ASSERT_X(qMetaTypeId<Ret>() != QMetaType::UnknownType, "Bindable::wait",
				   "Requested return type is not registered, please use the Q_DECLARE_METATYPE "
				   "macro to make it known to Qt's meta-object system");
		Q_ASSERT_X(method.returnType() == qMetaTypeId<Ret>() || QMetaType::hasRegisteredConverterFunction(method.returnType(), qMetaTypeId<Ret>()), "Bindable::wait",
				   qPrintable(QString("Requested return type (%1) is incompatible method return type (%2)")
								  .arg(QMetaType::typeName(qMetaTypeId<Ret>()),
									   QMetaType::typeName(method.returnType()))));
		const Qt::ConnectionType type = QThread::currentThread() == qApp->thread()
											? Qt::DirectConnection
											: Qt::BlockingQueuedConnection;
		Ret ret;
		const auto retArg =
			QReturnArgument<Ret>(QMetaType::typeName(qMetaTypeId<Ret>()),
								 ret); // because Q_RETURN_ARG doesn't work with templates...
		method.invoke(binding.receiver, type, retArg, Q_ARG(Params, params)...);
		return ret;
	}
};

// used frequently
Q_DECLARE_METATYPE(bool *)
