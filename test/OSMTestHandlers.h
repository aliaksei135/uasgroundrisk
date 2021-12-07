/*
 * OSMTestHandlers.h
 *
 *  Created by A.Pilko on 08/04/2021.
 */

#ifndef OSMTESTHANDLERS_H
#define OSMTESTHANDLERS_H
#include <osmium/geom/relations.hpp>
#include <osmium/handler.hpp>

#include <osmium/osm/area.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/way.hpp>

#include <iostream>

#include <vector>

#include <gtest/gtest.h>

using namespace osmium::handler;
using namespace osmium;

template <typename T>
class BaseTestHandler : public Handler
{
public:
	typedef std::function<const bool(const T&)> assertionFunctionType;

	std::vector<assertionFunctionType> assertionFunctions;
	std::vector<assertionFunctionType> expectationFunctions;

	/**
	 * Assert defined condition(s) are true for every OSMObject processed by the
	 * handler.
	 *
	 * The passed function should return true if the test has passed and false
	 * otherwise. A failed test will stop execution by generating a fatal test
	 * failure through GTest.
	 *
	 * @param func function to run for every OSMObject
	 */
	void addAssertionFunction(const assertionFunctionType& func)
	{
		assertionFunctions.emplace_back(func);
	}

	/**
	 * Identical to addAssertionFunction expect generates non fatal failures
	 * through GTest.
	 *
	 * @param func function to run for every OSMObject
	 */
	void addExpectationFunction(const assertionFunctionType& func)
	{
		expectationFunctions.emplace_back(func);
	}
};

class WayTestHandler : public BaseTestHandler<Way>
{
public:
	void way(const Way& way) const noexcept
	{
		for (const auto& func : assertionFunctions)
		{
			if (!func(way))
			{
				FAIL() << "Assertion for type " << way.type() << " with id " << way.id()
					<< " not met";
			}
		}

		for (const auto& func : expectationFunctions)
		{
			if (!func(way))
			{
				ADD_FAILURE() << "Expectation for type" << way.type() << " with id "
					<< way.id() << " not met";
			}
		}
	}
};

class NodeTestHandler : public BaseTestHandler<Node>
{
public:
	void node(const Node& node) const noexcept
	{
		for (const auto& func : assertionFunctions)
		{
			if (!func(node))
			{
				FAIL() << "Assertion for type " << node.type() << " with id "
					<< node.id() << " not met";
			}
		}

		for (const auto& func : expectationFunctions)
		{
			if (!func(node))
			{
				ADD_FAILURE() << "Expectation for type " << node.type() << " with id "
					<< node.id() << " not met";
			}
		}
	}
};

class AreaTestHandler : public BaseTestHandler<Area>
{
public:
	void area(const Area& area) const noexcept
	{
		for (const auto& func : assertionFunctions)
		{
			if (!func(area))
			{
				FAIL() << "Assertion for type " << area.type() << " with id "
					<< area.id() << " not met";
			}
		}

		for (const auto& func : expectationFunctions)
		{
			if (!func(area))
			{
				ADD_FAILURE() << "Expectation for type " << area.type() << " with id "
					<< area.id() << " not met";
			}
		}
	}
};
#endif // OSMTESTHANDLERS_H
