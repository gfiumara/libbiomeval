/*
 * This software was developed at the National Institute of Standards and
 * Technology (NIST) by employees of the Federal Government in the course
 * of their official duties. Pursuant to title 17 Section 105 of the
 * United States Code, this software is not subject to copyright protection
 * and is in the public domain. NIST assumes no responsibility whatsoever for
 * its use by other parties, and makes no guarantees, expressed or implied,
 * about its quality, reliability, or any other characteristic.
 */

#include <ostream>

#include <be_error_exception.h>
#include <be_time_timer.h>

BiometricEvaluation::Time::Timer::Timer() :
    _inProgress{false},
    _start{},
    _finish{}
{

}

BiometricEvaluation::Time::Timer::Timer(
    const std::function<void()> &func) :
    BiometricEvaluation::Time::Timer::Timer()
{
	this->time(func);
}

void
BiometricEvaluation::Time::Timer::start()
{
	if (this->_inProgress) {
		throw Error::StrategyError("Timing already in progress");
	}
	this->_inProgress = true;
	this->_start = BE_CLOCK_TYPE::now();
}

void
BiometricEvaluation::Time::Timer::stop()
{
	/* Get the time immediately */
	const auto stopTime = BE_CLOCK_TYPE::now();

	if (!this->_inProgress) {
		throw Error::StrategyError("Timing not in progress");
	}
	this->_finish = stopTime;
	this->_inProgress = false;
}

std::common_type_t<
    BiometricEvaluation::Time::Timer::BE_CLOCK_TYPE::time_point::duration,
    BiometricEvaluation::Time::Timer::BE_CLOCK_TYPE::time_point::duration>
BiometricEvaluation::Time::Timer::elapsedTimePoint()
    const
{
	if (this->_inProgress)
		throw Error::StrategyError{"Timing in progress"};

	return (this->_finish - this->_start);
}

BiometricEvaluation::Time::Timer&
BiometricEvaluation::Time::Timer::time(
    const std::function<void()> &func)
{
	if (func == nullptr)
		throw BiometricEvaluation::Error::StrategyError(
		    "Timing nullptr function");

	this->start();
	func();
	this->stop();

	return (*this);
}

std::ostream&
BiometricEvaluation::Time::operator<<(
    std::ostream &s,
    const BiometricEvaluation::Time::Timer &timer)
{
	return (s << timer.elapsedStr<std::chrono::microseconds>());
}

