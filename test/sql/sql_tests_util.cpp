//===----------------------------------------------------------------------===//
//
//                         Peloton
//
// sql_tests_util.cpp
//
// Identification: test/sql/sql_tests_util.cpp
//
// Copyright (c) 2015-16, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//
#include "sql/sql_tests_util.h"

#include "catalog/catalog.h"
#include "common/logger.h"
#include "executor/plan_executor.h"
#include "optimizer/rule.h"
#include "optimizer/simple_optimizer.h"
#include "parser/parser.h"
#include "planner/plan_util.h"
#include "tcop/tcop.h"

namespace peloton {

//===--------------------------------------------------------------------===//
// Utils
//===--------------------------------------------------------------------===//

namespace test {

// Show the content in the specific table in the specific database
// Note: In order to see the content from the command line, you have to
// turn-on LOG_TRACE.
void SQLTestsUtil::ShowTable(std::string database_name,
                             std::string table_name) {
  ExecuteSQLQuery("SELECT * FROM " + database_name + "." + table_name);
}

// Execute a SQL query end-to-end
Result SQLTestsUtil::ExecuteSQLQuery(
    const std::string query, std::vector<ResultType> &result,
    std::vector<FieldInfoType> &tuple_descriptor, int &rows_changed,
    std::string &error_message) {
  LOG_INFO("Query: %s", query.c_str());
  auto status = traffic_cop_.ExecuteStatement(query, result, tuple_descriptor,
                                              rows_changed, error_message);
  LOG_INFO("Statement executed. Result: %d", status);
  return status;
}

// Execute a SQL query end-to-end with the specific optimizer
Result SQLTestsUtil::ExecuteSQLQueryWithOptimizer(
    std::unique_ptr<optimizer::AbstractOptimizer> &optimizer,
    const std::string query, std::vector<ResultType> &result,
    std::vector<FieldInfoType> &tuple_descriptor, int &rows_changed,
    std::string &error_message) {
  auto &peloton_parser = parser::Parser::GetInstance();
  std::vector<type::Value> params;

  auto parsed_stmt = peloton_parser.BuildParseTree(query);
  auto plan = optimizer->BuildPelotonPlanTree(parsed_stmt);
  tuple_descriptor = std::move(
      traffic_cop_.GenerateTupleDescriptor(parsed_stmt->GetStatement(0)));
  auto result_format = std::move(std::vector<int>(tuple_descriptor.size(), 0));

  try {
    LOG_DEBUG("%s", planner::PlanUtil::GetInfo(plan.get()).c_str());
    auto status = traffic_cop_.ExecuteStatementPlan(plan.get(), params,
                                                    result, result_format);
    rows_changed = status.m_processed;
    LOG_INFO("Statement executed. Result: %d", status.m_result);
    return status.m_result;

  } catch (Exception &e) {
    error_message = e.what();
    return Result::RESULT_FAILURE;
  }
}

std::shared_ptr<planner::AbstractPlan> SQLTestsUtil::GeneratePlanWithOptimizer(
    std::unique_ptr<optimizer::AbstractOptimizer> &optimizer,
    const std::string query) {
  auto &peloton_parser = parser::Parser::GetInstance();

  auto parsed_stmt = peloton_parser.BuildParseTree(query);

  return optimizer->BuildPelotonPlanTree(parsed_stmt);
}

Result SQLTestsUtil::ExecuteSQLQuery(const std::string query,
                                     std::vector<ResultType> &result) {
  std::vector<FieldInfoType> tuple_descriptor;
  std::string error_message;
  int rows_changed;

  // execute the query using tcop
  auto status = traffic_cop_.ExecuteStatement(query, result, tuple_descriptor,
                                              rows_changed, error_message);

  return status;
}

Result SQLTestsUtil::ExecuteSQLQuery(const std::string query) {
  std::vector<ResultType> result;
  std::vector<FieldInfoType> tuple_descriptor;
  std::string error_message;
  int rows_changed;

  // execute the query using tcop
  auto status = traffic_cop_.ExecuteStatement(query, result, tuple_descriptor,
                                              rows_changed, error_message);

  return status;
}

tcop::TrafficCop SQLTestsUtil::traffic_cop_;

}  // namespace test
}  // namespace peloton
