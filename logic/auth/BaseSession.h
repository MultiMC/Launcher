#pragma once

#include <memory>

class BaseSession
{
public:
	virtual ~BaseSession() {}

	virtual bool invalidPassword() const = 0;
};
using SessionPtr = std::shared_ptr<BaseSession>;
