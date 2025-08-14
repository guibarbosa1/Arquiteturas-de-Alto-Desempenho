from mpi4py import MPI
import numpy as np
import time

def monte_carlo_pi_serial(n):
    """Versão serial do método de Monte Carlo para cálculo de π."""
    x = np.random.random(n)
    y = np.random.random(n)
    return np.count_nonzero(x**2 + y**2 <= 1.0)

def monte_carlo_pi_parallel(n_total, rank, size):
    """Versão paralela (MPI) do método de Monte Carlo."""
    n_local = n_total // size
    np.random.seed(seed=rank + int(MPI.Wtime() * 10000) % 100000)
    x = np.random.random(n_local)
    y = np.random.random(n_local)
    local_count = np.count_nonzero(x**2 + y**2 <= 1.0)
    total_count = comm.reduce(local_count, op=MPI.SUM, root=0)
    return total_count

# Inicialização MPI
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

# Número total de pontos (deve ser grande)
n = 10**7

# Apenas o processo 0 executa a versão serial
if rank == 0:
    start_serial = time.time()
    count_serial = monte_carlo_pi_serial(n)
    end_serial = time.time()
    pi_serial = 4 * count_serial / n
    time_serial = end_serial - start_serial
    print(f"[Serial] pi = {pi_serial}")
    print(f"[Serial] Tempo: {time_serial:.4f} s")

# Sincroniza todos os processos antes de iniciar o tempo paralelo
comm.Barrier()
start_parallel = MPI.Wtime()
count_parallel = monte_carlo_pi_parallel(n, rank, size)
end_parallel = MPI.Wtime()

# Processo 0 calcula e imprime resultados finais
if rank == 0:
    pi_parallel = 4 * count_parallel / n
    time_parallel = end_parallel - start_parallel
    speedup = time_serial / time_parallel
    print(f"[Paralelo] pi = {pi_parallel}")
    print(f"[Paralelo] Tempo com {size} processos: {time_parallel:.4f} s")
    print(f"[Speedup] Aceleração: {speedup:.2f}x")
