import unittest

from gunny_game import Tank, apply_shot, compute_damage, compute_landing_distance


class GunnyGameTests(unittest.TestCase):
    def test_compute_damage_thresholds(self):
        self.assertEqual(compute_damage(0.4), 45)
        self.assertEqual(compute_damage(1.5), 30)
        self.assertEqual(compute_damage(2.7), 18)
        self.assertEqual(compute_damage(4.0), 0)

    def test_compute_landing_distance_non_negative(self):
        self.assertGreaterEqual(compute_landing_distance(45, 20, -100), 0)

    def test_apply_shot_reduces_hp_when_hit(self):
        shooter = Tank("A", 2)
        target = Tank("B", 38)

        # Deterministic near-hit setup
        damage, _ = apply_shot(shooter, target, angle=45, power=18.78, wind=0)

        self.assertGreater(damage, 0)
        self.assertEqual(target.hp, 100 - damage)


if __name__ == "__main__":
    unittest.main()
