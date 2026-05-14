#!/usr/bin/env python3
"""A tiny Gunny-style turn-based terminal game."""

from __future__ import annotations

import math
import random

MAX_HP = 100
FIELD_WIDTH = 41
GRAVITY = 9.8


class Tank:
    def __init__(self, name: str, x: int) -> None:
        self.name = name
        self.x = x
        self.hp = MAX_HP



def clamp(value: float, low: float, high: float) -> float:
    return max(low, min(value, high))



def compute_landing_distance(angle_deg: float, power: float, wind: float) -> float:
    angle_rad = math.radians(angle_deg)
    base = (power * power * math.sin(2 * angle_rad)) / GRAVITY
    return max(0.0, base + wind)



def compute_damage(distance_error: float) -> int:
    if distance_error <= 1:
        return 45
    if distance_error <= 2:
        return 30
    if distance_error <= 3:
        return 18
    return 0



def parse_shot(prompt: str) -> tuple[float, float] | None:
    raw = input(prompt).strip().lower()
    if raw in {"q", "quit", "exit"}:
        return None
    parts = raw.replace(",", " ").split()
    if len(parts) != 2:
        raise ValueError("Please enter: <angle> <power>")
    angle = float(parts[0])
    power = float(parts[1])
    angle = clamp(angle, 10, 80)
    power = clamp(power, 10, 35)
    return angle, power



def render_field(player: Tank, enemy: Tank, impact_x: float | None = None) -> str:
    line = ["."] * FIELD_WIDTH
    line[player.x] = "P"
    line[enemy.x] = "E"
    if impact_x is not None:
        idx = int(round(clamp(impact_x, 0, FIELD_WIDTH - 1)))
        if line[idx] == ".":
            line[idx] = "*"
    return "".join(line)



def apply_shot(shooter: Tank, target: Tank, angle: float, power: float, wind: float) -> tuple[int, float]:
    distance = abs(target.x - shooter.x)
    landing = compute_landing_distance(angle, power, wind)
    impact_x = shooter.x + landing if shooter.x < target.x else shooter.x - landing
    distance_error = abs(abs(impact_x - shooter.x) - distance)
    damage = compute_damage(distance_error)
    target.hp = max(0, target.hp - damage)
    return damage, impact_x



def enemy_ai_shot(player: Tank, enemy: Tank, wind: float) -> tuple[float, float]:
    distance = abs(player.x - enemy.x)
    preferred_angle = random.uniform(30, 55)
    angle_rad = math.radians(preferred_angle)
    sin_component = max(math.sin(2 * angle_rad), 0.1)
    required_range = distance - wind
    if required_range <= 0:
        return preferred_angle, 10
    estimated_power = math.sqrt((required_range * GRAVITY) / sin_component)
    power = clamp(estimated_power + random.uniform(-2.0, 2.0), 10, 35)
    return preferred_angle, power



def print_status(player: Tank, enemy: Tank, wind: float, impact_x: float | None = None) -> None:
    print()
    print(render_field(player, enemy, impact_x))
    print(f"Wind: {wind:+.1f} | {player.name} HP: {player.hp} | {enemy.name} HP: {enemy.hp}")



def main() -> None:
    print("=== Gunny In Terminal ===")
    print("Enter angle and power (example: 45 25). Type q to quit.")

    player = Tank("You", 2)
    enemy = Tank("Enemy", FIELD_WIDTH - 3)

    turn = 1
    while player.hp > 0 and enemy.hp > 0:
        wind = random.uniform(-3.0, 3.0)
        print_status(player, enemy, wind)

        try:
            shot = parse_shot("Your shot (angle power): ")
        except ValueError as err:
            print(err)
            continue

        if shot is None:
            print("Game ended.")
            return

        angle, power = shot
        damage, impact_x = apply_shot(player, enemy, angle, power, wind)
        print_status(player, enemy, wind, impact_x)
        print(f"Turn {turn}: You dealt {damage} damage.")
        if enemy.hp <= 0:
            break

        enemy_angle, enemy_power = enemy_ai_shot(player, enemy, wind)
        enemy_damage, enemy_impact_x = apply_shot(enemy, player, enemy_angle, enemy_power, wind)
        print_status(player, enemy, wind, enemy_impact_x)
        print(
            f"Turn {turn}: Enemy shot at {enemy_angle:.1f}° / {enemy_power:.1f} power and dealt {enemy_damage} damage."
        )

        turn += 1

    print()
    if player.hp > 0:
        print("You win! 🎉")
    else:
        print("You lose. Try again!")


if __name__ == "__main__":
    main()
