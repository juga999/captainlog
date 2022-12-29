#include <captainlog/db.hpp>
#include <captainlog/utils.hpp>

using tl::make_unexpected;

namespace cl {

expected<Task, std::string> Db::task_from_json(const json& json_task)
{
        for (const auto& property : Task::REQUIRED_PROPERTIES) {
        if (!json_task.contains(property)) {
            return make_unexpected(std::string("Missing property: ") + property);
        }
    }

    auto schedule_res = TaskSchedule::create(
        json_task[Task::PROPERTY_START].get<std::string>(),
        json_task[Task::PROPERTY_STOP].get<std::string>());
    if (!schedule_res) {
        return make_unexpected(schedule_res.error());
    }

    std::string project = json_task[Task::PROPERTY_PROJECT].get<std::string>();
    std::string description = json_task[Task::PROPERTY_DESCRIPTION].get<std::string>();
    std::string tags_str;
    if (json_task.contains(Task::PROPERTY_TAGS)) {
        tags_str = cl::utils::join(json_task[Task::PROPERTY_TAGS].get<std::set<std::string>>(), ',');
    }
    std::string comment;
    if (json_task.contains(Task::PROPERTY_COMMENT)) {
        comment = json_task[Task::PROPERTY_COMMENT].get<std::string>();
    }

    Task task_without_id(schedule_res.value(), project, description, tags_str, comment);
    if (json_task.contains(Task::PROPERTY_ID)) {
        return Task(json_task[Task::PROPERTY_ID].get<int>(), std::move(task_without_id));
    } else {
        return task_without_id;
    }
}

}
