from typing import Iterable, Iterator, Any
import threading
import queue
import sys

class Comm:
    def get_size() -> int:
        raise NotImplemented
    def get_rank() -> int:
        raise NotImplemented
    def send(self, dest: int, data: Any):
        raise NotImplemented
    def recv(self) -> tuple[int, Any]:
        raise NotImplemented

class Task:
    """
    master process:
    task.setup(comm)
    item_list = list(task.produce())
    worker_pool.send(item_list)
    for result in worker_pool.recv():
        task.consume(result)
    task.finalize(comm)

    worker process:
    task.setup_worker()
    for item in master.recv():
        result = task.apply(item)
        master.send(result)
    task.finalize_worker()
    """
    def setup(self, comm: Comm | None = None):
        print("WARNING: setup has not been implemented", file=sys.stderr)
    def finalize(self):
        print("WARNING: finalize has not been implemented", file=sys.stderr)
    def produce(self) -> Iterable[Any]:
        raise NotImplemented
    def consume(self, result: Any):
        raise NotImplemented
    def apply(self, item: Any) -> Any:
        raise NotImplemented
    def setup_worker(self, comm: Comm | None = None):
        print("WARNING: setup_worker has not been implemented", file=sys.stderr)
    def finalize_worker(self):
        print("WARNING: finalize_worker has not been implemented", file=sys.stderr)

def run_task(task: Task, comm: Comm | None = None):
    # RUN IN SEQUENTIAL MODE
    if comm is None or comm.get_size() == 1:
        print("WARNING: running in sequential mode. Use mpiexec to run in parallel", file=sys.stderr)
        task.setup()
        task.setup_worker(comm)
        for item in task.produce():
            result = task.apply(item)
            task.consume(result)
        task.finalize_worker()
        task.finalize()
        return

    # RUN IN DISTRIBUTED MODE
    if comm.get_rank() == 0: # master
        task.setup(comm=comm)

        data_iter = iter(task.produce())
        q_emit = queue.Queue(maxsize=comm.get_size()-1) # queue of free workers
        q_send = queue.Queue(maxsize=comm.get_size()-1) # queue of messages to send
        q_recv = queue.Queue(maxsize=comm.get_size()-1) # queue of messages to recv
        for worker in range(1, comm.get_size()):
            q_emit.put(worker)

        def emit_task(q_emit: queue.Queue, q_send: queue.Queue, q_recv: queue.Queue):
            for data in data_iter:
                worker = q_emit.get()
                q_send.put((worker, data))
                
            for worker in range(1, comm.get_size()):
                q_send.put((worker, None)) # notify send_task that there is no more data to send

        def send_task(q_emit: queue.Queue, q_send: queue.Queue, q_recv: queue.Queue):
            while True:
                worker, data = q_send.get()
                if data is None: # no more data to send, break
                    q_recv.put(False) # notify recv_task that there is no more data to recv
                    break
                comm.send(dest=worker, data=data)
                q_recv.put(True)
        
        def recv_task(q_emit: queue.Queue, q_send: queue.Queue, q_recv: queue.Queue):
            while True:
                token = q_recv.get()
                if not token: # no more data to recv, break
                    break
                worker, data_out = comm.recv()
                task.consume(data_out)
                q_emit.put(worker)
        
        t_list = [
            threading.Thread(target=target, args=(q_emit, q_send, q_recv))
            for target in [emit_task, send_task, recv_task]
        ]
        for t in t_list:
            t.start()
        for t in t_list:
            t.join()
        # tell all workers to stop
        for worker in range(1, comm.get_size()):
            comm.send(dest=worker, data=None)
        
        task.finalize()

    else: # worker
        task.setup_worker(comm=comm)
        while True:
            srce, data = comm.recv()
            if data is None:
                break
            data_out = task.apply(data)
            comm.send(dest=0, data=data_out)
        task.finalize_worker()

class MPI_Comm(Comm):
    def __init__(self, MPI):
        self.MPI = MPI
        self.comm = self.MPI.COMM_WORLD
        self.size = self.comm.Get_size()
        self.rank = self.comm.Get_rank()
    
    def get_size(self) -> int:
        return self.size
    
    def get_rank(self) -> int:
        return self.rank

    def send(self, dest: int, data: Any):
        self.comm.send((self.rank, data), dest=dest)
    
    def recv(self) -> tuple[int, Any]:
        srce, data = self.comm.recv()
        return srce, data

if __name__ == "__main__":
    from tqdm import tqdm
    import time
    import random
    class ExampleTask(Task):
        def setup(self, comm: Comm | None = None):
            self.pbar = tqdm(total=26, desc="running ...")
        
        def finalize(self):
            self.pbar.close()
            print()

        def setup_worker(self, comm: Comm | None = None):
            self.signature = f"{comm.get_rank()}"

        def apply(self, item: Any) -> Any:
            assert isinstance(item, str), "item must be str"
            new_item = item.upper()
            time.sleep(0.1*random.random())
            return str(new_item) + "_" + self.signature
        
        def produce(self) -> Iterator[int]:
            return iter("abcdefghijklmnopqrstuvwxyz")
        
        def consume(self, result: Any):
            assert isinstance(result, str), "result must be str"
            self.pbar.update(1)
            print(result, end=" ")
    

    from mpi4py import MPI
    run_task(task=ExampleTask(), comm=MPI_Comm(MPI=MPI))




