"""3D DVF generation using ITK-Elastix (pure Python)."""
from pathlib import Path

def generate_dvf(fixed_path: Path, moving_path: Path, param_path: Path, output_path: Path) -> None:
    """Register moving to fixed, output DVF."""
    import itk

    fixed_path = Path(fixed_path)
    moving_path = Path(moving_path)
    param_path = Path(param_path)
    output_path = Path(output_path)

    fixed_img = itk.imread(str(fixed_path), itk.F)
    moving_img = itk.imread(str(moving_path), itk.F)

    param_obj = itk.ParameterObject.New()
    param_obj.AddParameterFile(str(param_path))

    ImageType = type(fixed_img)
    elastix = itk.ElastixRegistrationMethod[ImageType, ImageType].New()
    elastix.SetFixedImage(fixed_img)
    elastix.SetMovingImage(moving_img)
    elastix.SetParameterObject(param_obj)
    elastix.Update()

    transform_params = elastix.GetTransformParameterObject()
    transformix = itk.TransformixFilter[ImageType].New()
    transformix.SetMovingImage(moving_img)
    transformix.SetTransformParameterObject(transform_params)
    transformix.SetComputeDeformationField(True)
    transformix.Update()

    dvf = transformix.GetOutputDeformationField()
    itk.imwrite(dvf, str(output_path))
