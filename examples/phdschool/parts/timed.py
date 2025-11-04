from time import perf_counter

def timed(label=""):
    class _Ctx:
        def __enter__(self):
            self.t0 = perf_counter()
        def __exit__(self, *exc):
            dt = perf_counter() - self.t0
            print(f"{label}: {dt*1000:.2f} ms")
    return _Ctx()
