#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

// Meta Information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CEVAT GULEC");
MODULE_DESCRIPTION("a module that generates sub processs tree");

char *name;
int age;
int input_pid;
/*
 * module_param(foo, int, 0000)
 * The first param is the parameters name
 * The second param is its data type
 * The final argument is the permissions bits,
 * for exposing parameters in sysfs (if non-zero) at a later stage.
 */

//char *name;
//int age;

//module_param(name, charp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//MODULE_PARM_DESC(name, "name of the caller");

//module_param(age, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//MODULE_PARM_DESC(age, "age of the caller");

module_param(input_pid, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(input_pid, "input_pid of the caller");

// A function that runs when the module is first loaded
struct task_struct* task_by_pid(int pid);
static void subprocess_tree(struct task_struct *task, int level);

int simple_init(void) {

	/*
	struct task_struct *ts;

	ts = get_pid_task(find_get_pid(4), PIDTYPE_PID);

	printk("Hello from the kernel, user: %s, age: %d\n", name, age);
	printk("command: %s\n", ts->comm);
	return 0;AC
	*/
	struct pid *pid_struct;
    struct task_struct *task;

	//getting pid number as input
    pid_struct = find_get_pid(input_pid);
    task = pid_task(pid_struct, PIDTYPE_PID);

    if (task) {
        printk(KERN_INFO "Parent Process: %s(%d)\n", task->comm, task->pid);
        subprocess_tree(task, 0);
    } else {
        printk(KERN_INFO "No process found with PID %d\n", input_pid);
    }

    return 0;

}

struct task_struct* task_by_pid(int pid) {
    return pid_task(find_get_pid(pid), PIDTYPE_PID);
}


void subprocess_tree(struct task_struct *task, int level) {
    struct task_struct *child;
    struct list_head *list;

    struct timespec64 ts_boot, ts_creation;
	//system boot time
    ktime_get_boottime_ts64(&ts_boot);

    // process creation time
    ts_creation = timespec64_add(ts_boot, ns_to_timespec64(task->start_time));

	//print child
    printk(KERN_INFO "%*s(%d) %s, Creation Time: %lld.%09ld\n",
           level * 2, "", task->pid, task->comm,
           (long long)ts_creation.tv_sec, ts_creation.tv_nsec);


	//if no children, exit the function
    if (list_empty(&task->children)) {
        return; 
    }
	
	//recursive function for child
    list_for_each(list, &task->children) {
        child = list_entry(list, struct task_struct, sibling);
        subprocess_tree(child, level + 1); 
    }
}





// A function that runs when the module is removed
void simple_exit(void) {
	printk("Goodbye from the kernel, user: %s, age: %d\n", name, age);
}

module_init(simple_init);
module_exit(simple_exit);
