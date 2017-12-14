#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/slab.h>

#include <linux/mm.h>       /* mmap related stuff               */
#include <linux/sched.h>

#include <linux/semaphore.h> /* This for semaphore mechanism    */
#include <linux/jiffies.h>   /* This for get jiffies            */
#include <linux/workqueue.h> /* This for work_queue             */

#ifndef VM_RESERVED
#define VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#endif

static int program_count = 0;

// Code for Heartbeat (heart_rate_monitor.h)
struct dentry  *file1;
static DEFINE_SEMAPHORE(list_lock);    //initialize list_lock -> semaphore lock.

static void  heart_rate_monitor_work_handler(struct work_struct *data);

struct mmap_info {
    char *data;         /* the data */
    int reference;      /* how many times it is mmapped */     
};
static struct node *head = NULL;

typedef struct {
    int64_t beat;
    int tag;
    int64_t timestamp;
    int64_t global_rate;
    int64_t window_rate;
    int64_t instant_rate;
} heartbeat_record_t;

typedef struct {
    int pid;
    int64_t min_heartrate;
    int64_t max_heartrate;
    int64_t window_size;

    int64_t counter;
    int64_t buffer_depth;
    int64_t buffer_index;
    int64_t read_index;
    char    valid;
} HB_global_state_t;

typedef struct {
    HB_global_state_t* state;
    heartbeat_record_t* log;
    int* file;
    char filename[256];
} heart_rate_monitor_t;

struct node{
    pid_t pid;
    heart_rate_monitor_t *hrm;
    struct node *next;
};

static struct mmap_info *mmap_pointer;

int heart_rate_monitor_init(heart_rate_monitor_t* hrm, int pid) {

//    printk(KERN_INFO "heart_rate_monitor_init\n");
    hrm->state = (HB_global_state_t *)mmap_pointer->data; 
    hrm->log = (heartbeat_record_t *)(mmap_pointer->data + sizeof(HB_global_state_t));

    return 0;
}
void heart_rate_monitor_finish(heart_rate_monitor_t* heart) {

}

int hrm_get_current(heart_rate_monitor_t volatile * hb, heartbeat_record_t volatile * record) {
    if(hb->state->valid) {
        memcpy(record, &hb->log[hb->state->read_index], sizeof(heartbeat_record_t));
    }
    return !hb->state->valid;
}

int hrm_get_history(heart_rate_monitor_t volatile * hb, heartbeat_record_t volatile * record, int n) {
    if(hb->state->counter > hb->state->buffer_index) {
        memcpy(record,
                &hb->log[hb->state->buffer_index],
                (hb->state->buffer_index*hb->state->buffer_depth)*sizeof(heartbeat_record_t));
        memcpy(record + (hb->state->buffer_index*hb->state->buffer_depth),
                &hb->log[0],
                (hb->state->buffer_index)*sizeof(heartbeat_record_t));
        return hb->state->buffer_depth;
    }
    else {
        memcpy(record,
                &hb->log[0],
                hb->state->buffer_index*sizeof(heartbeat_record_t));
        return hb->state->buffer_index;
    }
}

int64_t hrm_get_global_rate(heart_rate_monitor_t volatile * hb) {//return : double => int64_t
      return hb->log[hb->state->read_index].global_rate;
}

int64_t hrm_get_windowed_rate(heart_rate_monitor_t volatile * hb) {//return : double => int64_t
      return hb->log[hb->state->read_index].window_rate;
}

int64_t hrm_get_min_rate(heart_rate_monitor_t volatile * hb) {//return : double => int64_t
      return hb->state->min_heartrate;
}

int64_t hrm_get_max_rate(heart_rate_monitor_t volatile * hb) {//return : double => int64_t
      return hb->state->max_heartrate;
}

int64_t hrm_get_window_size(heart_rate_monitor_t volatile * hb) {
      return hb->state->window_size;
}

//code for node list
void insert_node(pid_t pid){

    //down vs down_interruptible(&list_lock);
    struct node *node_pointer = NULL;
    down(&list_lock);

    node_pointer = kmalloc(sizeof(struct node), GFP_KERNEL);
    node_pointer->pid = pid;
    node_pointer->hrm = kmalloc(sizeof(heart_rate_monitor_t), GFP_KERNEL);
    node_pointer->next = NULL;
    heart_rate_monitor_init(node_pointer->hrm, pid);

    if(head == NULL){
        head = node_pointer;
//        printk(KERN_INFO "insert to head\n");
    }
    else{
        struct node *temp = head;
        while(temp->next != NULL){
            temp = temp->next;
        }
        temp->next = node_pointer;
    }
    up(&list_lock);
}

void remove_node(pid_t pid){

    struct node *target = NULL;
    struct node *parent = NULL;
    //down vs down_interruptible(&list_lock);
    
    down(&list_lock);

    target = head;
    parent = head;

    while(target->pid != pid && target != NULL){
        parent = target;
        target = target->next;
    }
    if(target == NULL)
        printk(KERN_INFO "there no target in the list.\n");
    else if(target == head)
        head = NULL;
    else if(target->next != NULL){
        parent->next = target->next;
    }else if(target->next == NULL){
        parent->next = NULL;
    }
    kfree(target);

    up(&list_lock);
}

void print_node(char *buffer){
    struct node *temp = head;
    int ret = 0;
    if(head == NULL)
        printk(KERN_INFO "head == NULL\n");
    while(temp != NULL){
        ret += sprintf(buffer + ret , "pid : %d, global_rate : %lld\n", temp->hrm->state->pid, hrm_get_global_rate(temp->hrm));
        temp = temp->next;
    }
//    printk(KERN_INFO "In Kernel -> global_rate : %llu\n", hrm_get_global_rate(head->hrm));
}

void print_node_work_queue(char *buffer){
    struct node *temp = head;
    int ret = 0;
    if(head == NULL){
        printk(KERN_INFO "head == NULL\n");
        memset(buffer, NULL, 1024);
    }
    while(temp != NULL){
        ret += sprintf(buffer + ret , "global_rate : %lld, tag : %d\n", hrm_get_global_rate(temp->hrm), temp->hrm->log[temp->hrm->state->buffer_index].tag);
        temp = temp->next;
    }
}

//code for work_queue
static DECLARE_DELAYED_WORK(work, heart_rate_monitor_work_handler);  //creating some work to defer.

static void  heart_rate_monitor_work_handler(struct work_struct *data){
    static char workqueue_buffer[2048];

    print_node_work_queue(workqueue_buffer);
    printk(KERN_INFO "work_queue message : %s", workqueue_buffer); // printk content of current list.
    schedule_delayed_work(&work, msecs_to_jiffies(1000));
}

/* keep track of how many times it is mmapped */
void mmap_open(struct vm_area_struct *vma){
    printk(KERN_INFO "mmap_open()\n");
    mmap_pointer = (struct mmap_info *)vma->vm_private_data;
    mmap_pointer->reference++;
}

void mmap_close(struct vm_area_struct *vma){
    printk(KERN_INFO "mmap_close()\n");
    mmap_pointer = (struct mmap_info *)vma->vm_private_data;
    mmap_pointer->reference--;
}

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf){
    struct page *page;
    printk(KERN_INFO "mmap_fault()\n");

    /* the data is in vma->vm_private_data */
    mmap_pointer = (struct mmap_info *)vma->vm_private_data;
    if (!mmap_pointer->data) {
        printk("no data\n");
        return -1;    
    }
    /* get the page */
    page = virt_to_page(mmap_pointer->data);

    /* increment the reference count of this page */
    get_page(page);
    vmf->page = page;                   
   
    insert_node(task_pid_nr(current));
    return 0;
}

struct vm_operations_struct mmap_vm_ops = {
    .open =     mmap_open,
    .close =    mmap_close,
    .fault =    mmap_fault,
};

int debugfs_mmap(struct file *filp, struct vm_area_struct *vma){
    printk(KERN_INFO "my_mmap()\n");
    mmap_pointer = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
    /* obtain new memory */
    mmap_pointer->data = (char *)get_zeroed_page(GFP_KERNEL);

    filp->private_data = mmap_pointer;
 
    vma->vm_ops = &mmap_vm_ops;
    vma->vm_flags |= VM_RESERVED;
    /* assign the file private data to the vm private data */
    vma->vm_private_data = filp->private_data;
    mmap_open(vma);
    return 0;
}

int my_close(struct inode *inode, struct file *filp){
    /* I  have to check if it freed or not!!!!!!!!!!!!!!!!!!!!!!1*/

    mmap_pointer = filp->private_data;
    /* obtain new memory */
    free_page((unsigned long)mmap_pointer->data);
    kfree(mmap_pointer);
    filp->private_data = NULL;

    remove_node(task_pid_nr(current));

    return 0;
}

int my_read(struct file *filp, char *buf, size_t count, loff_t *offp){

    int ret = 0;

    printk(KERN_INFO "my_read()\n");

    if(program_count == 0){
        print_node(buf);
        ret = sizeof(buf);
        return ret;    
    }else{
        return 0;
    }
}

int debugfs_open(struct inode *inode, struct file *filp){
    printk(KERN_INFO "my_open()\n");
//    sema_init(&list_lock, 1);
    return 0;
}

static int my_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg) {
	printk(KERN_INFO "my_ioctl()\n");
    
    free_page((unsigned long)mmap_pointer->data); //return to kernel the allocated page
    kfree(mmap_pointer);//return to kernel allocated mmap_info memory

    remove_node(task_pid_nr(current));//remove current node from the list
    
    return 0;
}

static const struct file_operations my_fops = {
    .open = debugfs_open,
    .read = my_read,
    //.release = my_close,
    .mmap = debugfs_mmap,
    .unlocked_ioctl = my_ioctl,
};

static int __init mmapexample_module_init(void)
{
    schedule_delayed_work(&work, msecs_to_jiffies(3000));
    file1 = debugfs_create_file("heartbeat", 0666, NULL, NULL, &my_fops);

    return 0;
}

static void __exit mmapexample_module_exit(void)
{
    printk(KERN_INFO "kernel module exit\n");
    cancel_delayed_work(&work);

    debugfs_remove(file1);
}

MODULE_LICENSE("GPL");

module_init(mmapexample_module_init);
module_exit(mmapexample_module_exit);
