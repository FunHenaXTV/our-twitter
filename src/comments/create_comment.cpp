#include "create_comment.hpp"
#include "../tools/verify_parameter.hpp"

#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/io/date.hpp>
#include <userver/utils/assert.hpp>

namespace just_post {

namespace {

class CreateComment final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-create-comment";

  CreateComment(const userver::components::ComponentConfig& config,
             const userver::components::ComponentContext& component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("postgres-db-1")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override {
    const auto& user_id = request.GetArg("user_id");
    const auto& post_id = request.GetArg("post_id");
    const auto& msg = request.RequestBody();

    int user_id_int = strtol(user_id.c_str(), NULL, 10);
    int post_id_int = strtol(post_id.c_str(), NULL, 10);

    if (user_id.empty() || !tools::IsValidId(user_id_int) || post_id.empty() || !tools::IsValidId(post_id_int)) {
      throw userver::server::handlers::ClientError(
          userver::server::handlers::ExternalBody{"Incorrect parameters\n"});
    }

    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT user_id "
        "FROM just_post_schema.users "
        "WHERE user_id=$1",
        user_id_int);

    if (result.Size() != 1) {
      throw userver::server::handlers::ClientError(
          userver::server::handlers::ExternalBody{
              "User with this ID does not exist\n"});
    }

    result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "SELECT post_id "
        "FROM just_post_schema.posts "
        "WHERE post_id=$1",
        post_id_int);

    if (result.Size() != 1) {
      throw userver::server::handlers::ClientError(
          userver::server::handlers::ExternalBody{
              "Post with this ID does not exist\n"});
    }

    result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        "INSERT INTO just_post_schema.comments(post_id, user_id, comment_body, date_of_comment) "
        "VALUES ($1, $2, $3, CURRENT_TIMESTAMP) "
        "ON CONFLICT DO NOTHING",
        post_id_int, user_id_int, msg);

    if (result.RowsAffected()) {
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kCreated);  // 201
      return "ok\n";
    }

    return "error\n";
  }

  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace
void AppendCreateComment(userver::components::ComponentList& component_list) {
  component_list.Append<CreateComment>();
}

}  // namespace just_post