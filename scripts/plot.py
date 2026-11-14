import pandas as pd
import matplotlib.pyplot as plt

# Читаем данные
df = pd.read_csv('../data/results.csv')

# Группируем и считаем среднее по 3 запускам для каждой конфигурации
df_mean = df.groupby('Config').mean().reset_index()

# Задаем красивые подписи для конфигураций
labels = {
    'A_5P_5ms': '5 фил., 5 мс',
    'B_5P_20ms': '5 фил., 20 мс',
    'C_7P_5ms': '7 фил., 5 мс',
    'D_7P_20ms': '7 фил., 20 мс'
}
df_mean['Label'] = df_mean['Config'].map(labels)

# --- ГРАФИК 1: Среднее время ожидания ---
plt.figure(figsize=(8, 5))
plt.bar(df_mean['Label'], df_mean['AvgWait'], color='skyblue', edgecolor='black')
plt.title('Среднее время ожидания вилок')
plt.xlabel('Конфигурация')
plt.ylabel('Время (мс)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Сохраняем первый график
plt.tight_layout()
plt.savefig('../report/plot_avg_wait.png', dpi=300)
print("График 1 сохранен: report/plot_avg_wait.png")

# --- ГРАФИК 2: Максимальное время ожидания ---
plt.figure(figsize=(8, 5))
plt.bar(df_mean['Label'], df_mean['MaxWait'], color='salmon', edgecolor='black')
plt.title('Максимальное время ожидания вилок')
plt.xlabel('Конфигурация')
plt.ylabel('Время (мс)')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Сохраняем второй график
plt.tight_layout()
plt.savefig('../report/plot_max_wait.png', dpi=300)
print("График 2 сохранен: report/plot_max_wait.png")

# Показываем графики на экране
plt.show()