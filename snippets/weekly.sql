with week_stats as (select 
sum((julianday(task_stop) - julianday(task_start))) as total,
strftime('%W',task_stop) as week_number
from tasks
where strftime('%W',task_stop) = '27'
)
select week_stats.week_number, task_project, round((sum((julianday(task_stop) - julianday(task_start))) / total)*100, 2) as pct
from tasks, week_stats
where strftime('%W',task_stop) = '27'
group by task_project
