import unittest
import math

from gunny_game import Tank, apply_shot, compute_damage, compute_landing_distance


class GunnyGameTests(unittest.TestCase):
    def test_compute_damage_thresholds(self):
        self.assertEqual(compute_damage(0.4), 45)
        self.assertEqual(compute_damage(1.5), 30)
        self.assertEqual(compute_damage(2.7), 18)
        self.assertEqual(compute_damage(4.0), 0)

    def test_compute_landing_distance_non_negative(self):
        self.assertGreaterEqual(compute_landing_distance(45, 20, -100), 0)

    def test_compute_landing_distance_known_value(self):
        expected = (20 * 20) / 9.8
        self.assertAlmostEqual(compute_landing_distance(45, 20, 0), expected, places=5)

    def test_apply_shot_reduces_hp_when_hit(self):
        shooter = Tank("A", 2)
        target = Tank("B", 38)
        power_for_direct_hit = math.sqrt((abs(target.x - shooter.x) * 9.8))
        damage, _ = apply_shot(shooter, target, angle=45, power=power_for_direct_hit, wind=0)

        self.assertGreater(damage, 0)
        self.assertEqual(target.hp, 100 - damage)


if __name__ == "__main__":
    unittest.main()
