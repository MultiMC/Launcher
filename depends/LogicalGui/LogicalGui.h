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
#include <QSemaphore>
#include <5.2.0/QtCore/private/qobject_p.h>
#include <memory>

class Bindable
{
public:
	Bindable(Bindable *parent = 0) : m_parent(parent)
	{
	}
	virtual ~Bindable()
	{
	}

	void setBindableParent(Bindable *parent)
	{
		m_parent = parent;
	}

	void bind(const QString &id, const QObject *receiver, const char *methodSignature)
	{
		auto mo = receiver->metaObject();
		Q_ASSERT_X(mo, "Bindable::bind",
				   "Invalid metaobject. Did you forget the QObject macro?");
		const QMetaMethod method = mo->method(mo->indexOfMethod(
			QMetaObject::normalizedSignature(methodSignature + 1).constData()));
		Q_ASSERT_X(method.isValid(), "Bindable::bind", "Invalid method signature");
		m_bindings.insert(id, Binding(receiver, method));
	}
	template <typename Func>
	void bind(const QString &id,
			  const typename QtPrivate::FunctionPointer<Func>::Object *receiver, Func slot)
	{
		typedef QtPrivate::FunctionPointer<Func> SlotType;
		m_bindings.insert(
			id, Binding(receiver,
						new QtPrivate::QSlotObject<
							Func, typename QtPrivate::List_Left<typename SlotType::Arguments,
																SlotType::ArgumentCount>::Value,
							typename SlotType::ReturnType>(slot)));
	}
	template <typename Func> void bind(const QString &id, Func slot)
	{
		typedef QtPrivate::FunctionPointer<Func> SlotType;
		m_bindings.insert(
			id,
			Binding(nullptr, new QtPrivate::QFunctorSlotObject<
								  Func, SlotType::ArgumentCount,
								  typename QtPrivate::List_Left<typename SlotType::Arguments,
																SlotType::ArgumentCount>::Value,
								  typename SlotType::ReturnType>(slot)));
	}
	void unbind(const QString &id)
	{
		m_bindings.remove(id);
	}

private:
	struct Binding
	{
		Binding(const QObject *receiver, const QMetaMethod &method)
			: receiver(receiver), method(method)
		{
		}
		Binding(const QObject *receiver, QtPrivate::QSlotObjectBase *object)
			: receiver(receiver), object(object)
		{
		}
		Binding()
		{
		}
		const QObject *receiver;
		QMetaMethod method;
		QtPrivate::QSlotObjectBase *object = nullptr;
	};
	QMap<QString, Binding> m_bindings;

	Bindable *m_parent;

protected:
	template <typename Ret, typename... Params> Ret wait(const QString &id, Params... params)
	{
		if (!m_bindings.contains(id) && m_parent)
		{
			return m_parent->wait<Ret, Params...>(id, params...);
		}
		Q_ASSERT(m_bindings.contains(id));
		QVariantList({qMetaTypeId<Params>()...});
		const Qt::ConnectionType type = QThread::currentThread() == qApp->thread()
				? Qt::DirectConnection
				: Qt::BlockingQueuedConnection;
		const auto binding = m_bindings[id];
		if (binding.object)
		{
			Ret ret;
			void *args[] = { &ret, const_cast<void *>(reinterpret_cast<const void *>(&params))... };
			int types[] = { qMetaTypeId<Params>()... };
			if (type == Qt::BlockingQueuedConnection)
			{
				QSemaphore semaphore;
				QMetaCallEvent *ev = new QMetaCallEvent(binding.object, nullptr, -1, sizeof...(Params), types, args, &semaphore);
				QCoreApplication::postEvent(const_cast<QObject *>(binding.receiver), ev);
				semaphore.acquire();
			}
			else
			{
				binding.object->call(const_cast<QObject *>(binding.receiver), args);
			}
			return ret;
		}
		else
		{
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
			Ret ret;
			const auto retArg =
					QReturnArgument<Ret>(QMetaType::typeName(qMetaTypeId<Ret>()),
										 ret); // because Q_RETURN_ARG doesn't work with templates...
			method.invoke(const_cast<QObject *>(binding.receiver), type, retArg, Q_ARG(Params, params)...);
			return ret;
		}
	}
};

// used frequently
Q_DECLARE_METATYPE(bool *)
