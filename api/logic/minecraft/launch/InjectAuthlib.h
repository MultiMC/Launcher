/* Copyright 2013-2021 MultiMC Contributors
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
#include <QObjectPtr.h>
#include <LoggedProcess.h>
#include <java/JavaChecker.h>
#include <net/Mode.h>
#include <net/NetJob.h>

struct AuthlibInjector
{
  QString javaArg;

  AuthlibInjector(const QString arg)
  {
    javaArg = std::move(arg);
    qDebug() << "NEW INJECTOR" << javaArg;
  }
};

typedef std::shared_ptr<AuthlibInjector> AuthlibInjectorPtr;

// FIXME: stupid. should be defined by the instance type? or even completely abstracted away...
class InjectAuthlib : public LaunchStep
{
  Q_OBJECT
public:
  InjectAuthlib(LaunchTask *parent, AuthlibInjectorPtr *injector);
  virtual ~InjectAuthlib(){};

  void executeTask() override;
  bool canAbort() const override;
  void proceed() override;

  void setAuthServer(QString server)
  {
    m_authServer = server;
  };

public slots:
  bool abort() override;

private slots:
  void onVersionDownloadSucceeded();
  void onDownloadSucceeded();
  void onDownloadFailed(QString reason);

private:
  shared_qobject_ptr<Task> jobPtr;
  bool m_aborted = false;

  QString m_versionName;
  QString m_authServer;
  AuthlibInjectorPtr *m_injector;
};
