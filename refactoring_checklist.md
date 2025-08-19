# PBBD Refactoring Checklist

## 1. Router Integration
- [ ] Copy `bramus/router` from `pbbd_v1/vendor` to `app/Vendor/`.
- [ ] Update `app/Core/Autoloader.php` to include the `Bramus\\Router` namespace.
- [ ] Create a new entry point `public_html/pbbd.php` for PBBD routes.
- [ ] Modify `.htaccess` to direct PBBD routes to `pbbd.php`.

## 2. Controller Refactoring
- [ ] Update `SystemBuilderController` to use main app's `BaseController`.
- [ ] Implement proper validation in `SystemBuilderController`.
- [ ] Implement proper error handling in `SystemBuilderController`.
- [ ] Update `PBBDDocumentController` to use main app's `BaseController`.
- [ ] Implement proper validation in `PBBDDocumentController`.
- [ ] Implement proper error handling in `PBBDDocumentController`.

## 3. Model Refactoring
- [ ] Ensure all PBBD models (`PrimitiveModel`, `BlockModel`, etc.) use the main app's database connection.
- [ ] Ensure all PBBD models extend `App\Core\Model`.

## 4. UI Integration
- [ ] Update System Builder views to extend `admin/layout/adminLayout`.
- [ ] Replace raw HTML forms with the application's form components.
- [ ] Update Document Editor views to extend `admin/layout/adminLayout`.
- [ ] Replace raw HTML forms in Document Editor with application's form components.

## 5. Cleanup
- [ ] Delete the `pbbd_v1` directory.
- [ ] Remove any temporary or unused files.
