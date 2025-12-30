# 2025-12-30T17:45:12.422857
import vitis

client = vitis.create_client()
client.set_workspace(path="fitness_function")

comp = client.get_component(name="fitness_hls")
comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

comp.run(operation="C_SIMULATION")

